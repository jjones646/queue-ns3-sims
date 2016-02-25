/*
 *
 *  ECE 6110
 *  Project 1
 */

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-helper.h"
#include "ns3/csma-star-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

#define LOG_COMPONENT_NAME "QueueSims"

// Set the documentation name
NS_LOG_COMPONENT_DEFINE (LOG_COMPONENT_NAME);

namespace
{
const int TCP_SERVER_BASE_PORT = 8080;
const int nFlowBytes = 1000;
}

class GoodputTracker
{
public:
    GoodputTracker () :
            recvCount (0), port (0), isValid (true)
    {
    }
    ;
    GoodputTracker (const Time& t) :
            recvCount (0), port (0), startTime (t), isValid (true)
    {
    }
    ;

    size_t recvCount;
    int port;
    Time startTime;
    Time endTime;
    bool isValid;
};

// global array of GoodputTracker objects
std::vector<GoodputTracker> goodputs;

void TrackGoodput (std::string context, Ptr<const Packet> p, const Address& address)
{
    // Get the id of the received packed
    size_t idIndex = context.find ("ApplicationList/");
    // the tcp sink's id that this callback is being executed for will
    // be 16 characters after the index that's found from the previous command
    idIndex += 16;

    // Increment the correct goodput tracker byte count
    if (idIndex != std::string::npos)
    {
        int flowId;
        // determine which tcp connection this callback was invoked for
        std::istringstream (std::string (context.substr (idIndex, 1))) >> flowId;
        if (goodputs.at (flowId).isValid == true)
        {
            goodputs.at (flowId).recvCount += p->GetSize ();
            // NS_LOG(LOG_DEBUG, "Sink RX->\tFlow: " << flowId
            //        << "\tFrom: " << InetSocketAddress::ConvertFrom(address).GetIpv4() << ":" << InetSocketAddress::ConvertFrom(address).GetPort()
            //        << "\tSize: " << p->GetSize() << " bytes"
            //        << "\tRecv: " << goodputs.at(flowId).recvCount << " bytes");

            // If we're at or past the limit, set the stop time for the tcp flow
            if (goodputs.at (flowId).recvCount >= nFlowBytes)
            {
                goodputs.at (flowId).endTime = Simulator::Now ();
                // set the object to be invalid so we won't increment the counter anymore
                goodputs.at (flowId).isValid = false;
            }
        }
    }
}

NetDeviceContainer CreateCsmaStar (size_t num_links)
{
    if (num_links < 2)
    {
        NS_LOG (LOG_ERROR, "Can not create star topology subnet with " << num_links << " device!");
    }
    else
    {
        NS_LOG (LOG_DEBUG, "Creating " << num_links << " link CSMA star topology");
    }

    NodeContainer nodes;
    nodes.Create (num_links);

    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
    csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
    CsmaStarHelper star (num_links, csma);

    NetDeviceContainer devs;

    for (size_t i = 0; i < star.GetSpokeDevices ().GetN (); ++i)
    {
        Ptr < Channel > channel = star.GetSpokeDevices ().Get (i)->GetChannel ();
        Ptr < CsmaChannel > csmaChannel = channel->GetObject<CsmaChannel> ();
        NodeContainer nn;
        nn.Create (2);
//        fillNodes.Add (nn);
//        NodeContainer fillNodes;
        devs.Add (csma.Install (nn, csmaChannel));
    }

    return devs;
}

NetDeviceContainer InitNetNodes ()
{
    // I know the below line makes no sense, it's just for easier
    // reading of the code in this function
//    std::vector < Ptr<NetDeviceContainer> > allNodes;

    // Create the container used in simulation for
    // representing the computers
    NS_LOG (LOG_DEBUG, "Creating " << 4 << " CSMA stars");
//    for (size_t i = 0; i < 4; ++i)
//        allNodes.push_back ();

    NetDeviceContainer nodesA = CreateCsmaStar (4);
    NetDeviceContainer nodesB = CreateCsmaStar (4);
    NetDeviceContainer nodesC = CreateCsmaStar (4);
    NetDeviceContainer nodesD = CreateCsmaStar (4);
    NetDeviceContainer nodesCore = CreateCsmaStar (2);

    NS_LOG (LOG_DEBUG, "Creating CSMA dumbbell link");
//    allNodes.push_back (CreateCsmaStar (2));

    // Create 3 point-to-point links
    NS_LOG (LOG_DEBUG, "Linking the CSMA stars into a larger overall topology");

    PointToPointHelper linkA, linkB, linkC, linkD, linkCore;

    // Set their attributes
    linkA.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    linkA.SetChannelAttribute ("Delay", StringValue ("10ms"));

    linkB.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
    linkB.SetChannelAttribute ("Delay", StringValue ("20ms"));

    linkC.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    linkC.SetChannelAttribute ("Delay", StringValue ("10ms"));

    linkD.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    linkD.SetChannelAttribute ("Delay", StringValue ("10ms"));

    linkCore.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    linkCore.SetChannelAttribute ("Delay", StringValue ("10ms"));

    // ===== Network Devices =====
    NS_LOG (LOG_DEBUG, "Creating network interface devices");

    NetDeviceContainer devicesA, devicesB, devicesC, devicesD, devicesCore;

    // Last vector element holds the nodes for the dumbbell link
    devicesA = linkA.Install (nodesA.Get (0), nodesCore.Get (0));
    devicesB = linkB.Install (nodesB.Get (0), nodesCore.Get (0));
    devicesC = linkC.Install (nodesC.Get (0), nodesCore.Get (1));
    devicesD = linkD.Install (nodesD.Get (0), nodesCore.Get (1));
    // Now, bring the two halves together
    devicesCore = linkCore.Install (nodesCore.Get (0), nodesCore.Get (1));

    // Now that it's constructed & the code is easy to follow, let's push it all
    // into contiguous memory and return it as a NetDeviceContainer. We'll place the
    // dumbbell nodes at the end for indexing at intervals of 4 for the startpoint LANs
    devicesA.Add (devicesB);
    devicesA.Add (devicesC);
    devicesA.Add (devicesD);
    devicesA.Add (devicesCore);

    return devicesA;
}

void SetSimConfigs (const std::string& xml_file)
{
    /*
     * Read in the XML configuration specified
     */
    Config::SetDefault ("ns3::ConfigStore::Filename", StringValue (xml_file));
    Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Load"));
    Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("Xml"));
    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults ();

    // Set the log levels for this module
    LogComponentEnable (LOG_COMPONENT_NAME, LOG_LEVEL_ALL);

    // Other logging modules enabled
    LogComponentEnable ("CsmaOneSubnetExample", LOG_LEVEL_INFO);

    // 1 ns time resolution, the default value
    Time::SetResolution (Time::NS);
}

int main (int argc, char* argv[])
{
    // Parse any command line arguments
    CommandLine cmd;

    size_t nFlows = 1;
//    nFlowBytes = 1000;
    std::string pcapFn = "tcp-trace-results";
    bool traceEN = false;
    std::string xml_fn ("/home/jonathan/Documents/queue-sims/ns3-config1.xml");

    cmd.AddValue ("nFlows", "Number of simultaneous TCP flows", nFlows);
    cmd.AddValue ("trace", "Enable/Disable dumping the trace at the TCP sink", traceEN);
    cmd.AddValue ("traceFile", "Base name given to where the results are saved when enabled", pcapFn);
    cmd.AddValue ("xml", "The name for the XML configuration file.", xml_fn);
    cmd.Parse (argc, argv);

    // Initial simulation configurations
    SetSimConfigs (xml_fn);

    // Star CSMA LANs for every 4 node indexes. 2 back nodes make up the dumbbell link
    NetDeviceContainer nodes = InitNetNodes ();

    // ===== Internet Stack Assignment =====
    // Set IPv4, IPv6, UDP, & TCP stacks to all nodes in the simulation
    NS_LOG (LOG_DEBUG, "Setting simulation to use IPv4, IPv6, UDP, & TCP stacks");
    InternetStackHelper stack;
    stack.InstallAll ();

    // ===== IPv4 Addresses ======
    NS_LOG (LOG_DEBUG, "Assigning IPv4 addresses");
    Ipv4AddressHelper addrsA, addrsB, addrsC, addrsD, addrsCore;

    // Define the addresses
    addrsA.SetBase ("10.0.0.0", "255.255.255.0");
    addrsB.SetBase ("10.64.0.0", "255.255.255.0");
    addrsC.SetBase ("10.128.0.0", "255.255.255.0");
    addrsD.SetBase ("10.192.0.0", "255.255.255.0");
    addrsCore.SetBase ("192.168.0.0", "255.255.255.192");

    // Assign the addresses to devices, every thing is very clear here on purpose
    Ipv4InterfaceContainer nics;
    nics = addrsA.Assign (nodes.Get (0));
    nics.Add (addrsA.Assign (nodes.Get (1)));
    nics.Add (addrsA.Assign (nodes.Get (2)));
    nics.Add (addrsA.Assign (nodes.Get (3)));
    nics.Add (addrsB.Assign (nodes.Get (4)));
    nics.Add (addrsB.Assign (nodes.Get (5)));
    nics.Add (addrsB.Assign (nodes.Get (6)));
    nics.Add (addrsB.Assign (nodes.Get (7)));
    nics.Add (addrsC.Assign (nodes.Get (8)));
    nics.Add (addrsC.Assign (nodes.Get (9)));
    nics.Add (addrsC.Assign (nodes.Get (10)));
    nics.Add (addrsC.Assign (nodes.Get (11)));
    nics.Add (addrsD.Assign (nodes.Get (12)));
    nics.Add (addrsD.Assign (nodes.Get (13)));
    nics.Add (addrsD.Assign (nodes.Get (14)));
    nics.Add (addrsD.Assign (nodes.Get (15)));
    nics.Add (addrsCore.Assign (nodes.Get (16)));
    nics.Add (addrsCore.Assign (nodes.Get (17)));

    // Enable IPv4 routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // ===== Application Client(s) =====
    // Create an application to send TCP data to the server
    NS_LOG (LOG_DEBUG, "Creating " << nFlows << " TCP flow" << (nFlows == 1 ? "" : "s"));

    ApplicationContainer udpApps;
    std::vector < std::pair<size_t, size_t> > udpConvs;
    udpConvs.push_back (std::make_pair (1, 8));
    uint16_t port = 9;

//    for(size_t i = 0; i < udpConvs.size(); ++i)
    for (auto const& c : udpConvs)
    {
        OnOffHelper onoff ("ns3::UdpSocketFactory", Address (InetSocketAddress (nics.GetAddress (c.first), port)));
        udpApps.Add (onoff.Install (nodes.Get (c.second)));
    }
    // Start the application
    udpApps.Start (Seconds (1.0));
    udpApps.Stop (Seconds (10.0));
    // Create an optional packet sink to receive these packets
//    PacketSinkHelper sink ("ns3::UdpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
//    app = sink.Install (nodes.Get (1));
//    app.Start (Seconds (0.0));

    // App container for holding all of the TCP flows
//    ApplicationContainer clientApps;
//
//    // Create the random variable object for setting the initial flow start times
//    for (size_t i = 0; i < nFlows; ++i)
//    {
//        Ptr < UniformRandomVariable > randVar = CreateObject<UniformRandomVariable> ();
//        NS_LOG (LOG_DEBUG, "Creating TCP source flow to " << nics.GetAddress (0) << ":" << TCP_SERVER_BASE_PORT + i);
//
//        Address tcpSinkAddr (InetSocketAddress (nicsA.GetAddress (0), TCP_SERVER_BASE_PORT + i));
//
//        BulkSendHelper tcpSource ("ns3::TcpSocketFactory", tcpSinkAddr);
//        ApplicationContainer tcpFlow;
//
//        // Set the attributes for how we send the data
//        tcpSource.SetAttribute ("MaxBytes", UintegerValue (nFlowBytes));
//
//        // Sending to the server node & starting at the last node in our nodes list
//        tcpFlow = tcpSource.Install (nodes.Get (nodes.GetN () - 1));
//
//        // Set the starting time to some uniformly distributed random time between 0.0s and 0.1s
//        goodputs.push_back (
//                GoodputTracker (Seconds (0.1 * (randVar->GetValue (0, randVar->GetMax ()) / randVar->GetMax ()))));
//        tcpFlow.Start (goodputs.back ().startTime);
//
//        // Set the TCP port we track for this flow
//        goodputs.back ().port = TCP_SERVER_BASE_PORT + i;
//
//        // Add this app container to the object holding all of our source flow apps
//        clientApps.Add (tcpFlow);
//    }

//    // ===== Application Server(s) =====
//    ApplicationContainer serverApps;
//
//    for (size_t i = 0; i < nFlows; ++i)
//    {
//        // Create a TCP packet sink
//        NS_LOG (LOG_DEBUG, "Creating TCP sink on " << nics.GetAddress (0) << ":" << goodputs.at (i).port);
//
//        // The TCP sink address
//        Address tcpSinkAddr (InetSocketAddress (nics.GetAddress (0), goodputs.at (i).port));
//        PacketSinkHelper tcpSink ("ns3::TcpSocketFactory", tcpSinkAddr);
//
//        // Assign it to the list of app servers
//        serverApps.Add (tcpSink.Install (nodes.Get (0)));
//    }
//
//    // Set all of the sink start times to 0
//    serverApps.Start (Seconds (0.0));

    // Set the advertise window by setting the receiving end's max RX buffer. Not doing this for
    // some reason causes ns-3 to not adheer to the "MaxWindowSize" set earilier?
//    Config::Set ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/*/RcvBufSize", UintegerValue (winSize));

    // Set the trace callback for receiving a packet at the sink destination
//    Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback (&TrackGoodput));

    // Set up tracing on the sink node if enabled
//    if (traceEN == true) linkA.EnablePcap (pcapFn, devicesA.Get (0), true);

//	AnimationInterface anim("anim.xml");
//	anim.SetConstantPosition(nodes.Get(0), 1.0, 2.0);

    // Run the simulation
    NS_LOG (LOG_INFO, "Starting simulation");
    Simulator::Run ();

    // Get the final simulation runtime
    Time sim_endTime = Simulator::Now ();

    Simulator::Destroy ();

    NS_LOG (LOG_INFO, "Simulation complete");

    // Print out the overall simulation runtime
    std::cout << "Total time: " << Seconds (sim_endTime);

    return 0;
}

