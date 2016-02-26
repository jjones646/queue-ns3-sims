/*
 *  ECE 6110
 *  Project 2
 */

#include <algorithm>
#include <numeric>
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

typedef std::pair<NodeContainer, NetDeviceContainer> topology_t;

// begin/end methods for a standard array
template<typename T, size_t N>
T* begin (T (&arr)[N])
{
	return &arr[0];
}
template<typename T, size_t N>
T* end (T (&arr)[N])
{
	return &arr[0] + N;
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

	// 1 ns time resolution, the default value
	Time::SetResolution (Time::NS);
}

int main (int argc, char* argv[])
{
	// Parse any command line arguments
	CommandLine cmd;
	std::string pcapFn = "queue-trace-results";
	bool traceEN = false;
	std::string xml_fn ("/home/jonathan/Documents/queue-sims/ns3-config1.xml");

	cmd.AddValue ("trace", "Enable/Disable dumping the trace at the TCP sink", traceEN);
	cmd.AddValue ("traceFile", "Base name given to where the results are saved when enabled", pcapFn);
	cmd.AddValue ("xml", "The name for the XML configuration file.", xml_fn);
	cmd.Parse (argc, argv);

	const double endTime = 10.0;

	// Initial simulation configurations
	SetSimConfigs (xml_fn);

	/*
	 * Vector representing the number of subnodes connected
	 * to each subnode.
	 */
	size_t nodesPerSpoke = 4;
	size_t sss[] = { 4, 4, 4, 4 };
	std::vector < size_t > starSpokes (begin (sss), end (sss));

	/*
	 * Get a count for the total number of nodes in use.
	 */
	size_t totalNodes = 0;
	for (size_t i = 0; i < starSpokes.size (); ++i)
		totalNodes += starSpokes.at (i);

	/*
	 * Where all the endpoint nodes in the topology are held.
	 */
	NodeContainer nodes;
	NetDeviceContainer devs;

	/*
	 * Create the container used in simulation for
	 * representing the computers.
	 */
	NS_LOG (LOG_DEBUG, "Creating " << starSpokes.size () << " CSMA stars");

	/*
	 * Where the CSMA intermediate hubs are held.
	 */
	NodeContainer hubNodes;

	/*
	 * Iterate over all star net devices.
	 */
	for (size_t i = 0; i < starSpokes.size (); ++i) {
		const size_t numSpokes = starSpokes.at (i);
		NS_LOG (LOG_DEBUG,
				"Creating a " << numSpokes << " spoke CSMA star with " << nodesPerSpoke << " nodes on each spoke");

		/*
		 * Create a CSMA Star net device.
		 */
		CsmaHelper csma;
		csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("500kb/s")));
		csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
		CsmaStarHelper star (numSpokes, csma);

		for (size_t j = 0; j < star.GetSpokeDevices ().GetN (); ++j) {
			/*
			 * Grab the CSMA channel from the current spoke.
			 */
			Ptr < Channel > channel = star.GetSpokeDevices ().Get (j)->GetChannel ();
			Ptr < CsmaChannel > csmaChannel = channel->GetObject<CsmaChannel> ();

			/*
			 * Place somes nodes on each spoke of the star.
			 */
			NodeContainer nn;
			nn.Create (nodesPerSpoke);

			/*
			 * Add the spoke to the overall node container.
			 */
			nodes.Add (nn);

			NS_LOG (LOG_DEBUG, "Currently " << nodes.GetN () << " total nodes");

			devs.Add (csma.Install (nn, csmaChannel));
		}

		// Add the hub to the list for storing all hub nodes
		hubNodes.Add (star.GetHub ());
	}

	/*
	 * Create point-to-point links connecting all of the stars together.
	 */
	NodeContainer coreNodes;
	NetDeviceContainer coreDevs;

	NS_LOG (LOG_DEBUG, "Creating nodes for middle dumbbell link");
	coreNodes.Create (2);

	NodeContainer n_A = NodeContainer (hubNodes.Get (0), coreNodes.Get (0));
	NodeContainer nA = NodeContainer (hubNodes.Get (0));
	for (size_t i = 0; i < nodesPerSpoke * starSpokes.at (0); ++i)
		nA.Add (nodes.Get (i));

	NodeContainer n_B = NodeContainer (hubNodes.Get (1), coreNodes.Get (0));
	NodeContainer nB = NodeContainer (hubNodes.Get (1));
	for (size_t i = 0; i < nodesPerSpoke * starSpokes.at (1); ++i)
		nB.Add (nodes.Get (i + starSpokes.front () * 1));

	NodeContainer n_C = NodeContainer (hubNodes.Get (2), coreNodes.Get (1));
	NodeContainer nC = NodeContainer (hubNodes.Get (2));
	for (size_t i = 0; i < nodesPerSpoke * starSpokes.at (2); ++i)
		nC.Add (nodes.Get (i + starSpokes.front () * 2));

	NodeContainer n_D = NodeContainer (hubNodes.Get (3), coreNodes.Get (1));
	NodeContainer nD = NodeContainer (hubNodes.Get (3));
	for (size_t i = 0; i < nodesPerSpoke * starSpokes.at (3); ++i)
		nD.Add (nodes.Get (i + starSpokes.front () * 3));

	PointToPointHelper linkA, linkB, linkC, linkD, linkCore;

	/*
	 * Set the attributes for the links between the star subnets and
	 * for the dumbbell link.
	 */
//	linkA.SetQueue ("ns3::DropTailQueue");
//	linkB.SetQueue ("ns3::DropTailQueue");
//	linkC.SetQueue ("ns3::DropTailQueue");
//	linkD.SetQueue ("ns3::DropTailQueue");
//	linkCore.SetQueue ("ns3::DropTailQueue");
	linkCore.SetQueue ("ns3::RedQueue");
	linkA.SetQueue ("ns3::RedQueue");
	linkB.SetQueue ("ns3::RedQueue");
	linkD.SetQueue ("ns3::RedQueue");
	linkC.SetQueue ("ns3::RedQueue");

	linkA.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
	linkA.SetChannelAttribute ("Delay", StringValue ("8ms"));

	linkB.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
	linkB.SetChannelAttribute ("Delay", StringValue ("8ms"));

	linkC.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
	linkC.SetChannelAttribute ("Delay", StringValue ("8ms"));

	linkD.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
	linkD.SetChannelAttribute ("Delay", StringValue ("8ms"));

	linkCore.SetDeviceAttribute ("DataRate", StringValue ("8Mbps"));
	linkCore.SetChannelAttribute ("Delay", StringValue ("10ms"));

	/*
	 * Create the network devices.
	 */
	NetDeviceContainer d_A, d_B, d_C, d_D;

	NS_LOG (LOG_DEBUG, "Connecting n_A");
	d_A.Add (linkA.Install (n_A));

	NS_LOG (LOG_DEBUG, "Connecting n_B");
	d_B.Add (linkB.Install (n_B));

	NS_LOG (LOG_DEBUG, "Connecting n_C");
	d_C.Add (linkC.Install (n_C));

	NS_LOG (LOG_DEBUG, "Connecting n_D");
	d_D.Add (linkD.Install (n_D));

	NS_LOG (LOG_DEBUG, "Creating network interface devices for " << coreNodes.GetN () << " coreNodes");
	coreDevs.Add (linkCore.Install (coreNodes));

	// Set IPv4, IPv6, UDP, & TCP stacks to all nodes in the simulation
	NS_LOG (LOG_DEBUG, "Setting simulation to use IPv4, IPv6, UDP, & TCP stacks");
	InternetStackHelper stack;
	stack.InstallAll ();

	NS_LOG (LOG_DEBUG, "Assigning IPv4 addresses for " << devs.GetN () << " devices");
	Ipv4AddressHelper ipv4;
	ipv4.SetBase ("10.4.0.0", "255.255.255.0");
	ipv4.Assign (d_A);

	ipv4.SetBase ("10.8.64.0", "255.255.255.0");
	ipv4.Assign (d_B);

	ipv4.SetBase ("10.4.128.0", "255.255.255.0");
	ipv4.Assign (d_C);

	ipv4.SetBase ("10.8.192.0", "255.255.255.0");
	ipv4.Assign (d_D);

	ipv4.SetBase ("57.20.43.0", "255.255.255.0");
	ipv4.Assign (coreDevs);

	ipv4.SetBase ("94.0.0.0", "255.255.255.0");
	for (size_t i = 0; i < nA.GetN (); ++i)
		ipv4.Assign (nA.Get (i)->GetDevice (0));

	ipv4.SetBase ("94.1.0.0", "255.255.255.0");
	for (size_t i = 0; i < nB.GetN (); ++i)
		ipv4.Assign (nB.Get (i)->GetDevice (0));

	ipv4.SetBase ("94.2.0.0", "255.255.255.0");
	for (size_t i = 0; i < nC.GetN (); ++i)
		ipv4.Assign (nC.Get (i)->GetDevice (0));

	ipv4.SetBase ("94.3.0.0", "255.255.255.0");
	for (size_t i = 0; i < nD.GetN (); ++i)
		ipv4.Assign (nD.Get (i)->GetDevice (0));

	// Create traffic sending applications
	ApplicationContainer udpApps, tcpApps, udpSinkApps, tcpSinkApps;

	const uint16_t udpPort = 9, tcpPort = 8080;

	// Helper objects for creating the TCP & UDP sinks
	PacketSinkHelper udpSink ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), udpPort));
	PacketSinkHelper tcpSink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), tcpPort));

	// Create the UDP traffic
	NS_LOG (LOG_DEBUG, "Constructing UDP traffic");
	for (size_t i = 1; i < nA.GetN (); ++i) {
		Ptr < Node > nodeSrc = nA.Get (i);
		Ptr < Ipv4 > ipSrc = nodeSrc->GetObject<Ipv4> ();
		Ipv4Address addrSrc = ipSrc->GetAddress (1, 0).GetLocal ();

		Ptr < Node > nodeDst = nC.Get (i);
		Ptr < Ipv4 > ipDst = nodeDst->GetObject<Ipv4> ();
		Ipv4Address addrDst = ipDst->GetAddress (1, 0).GetLocal ();

		// Construct the sink end of the UDF flow
		PacketSinkHelper udpSink ("ns3::UdpSocketFactory", Address (InetSocketAddress (addrDst, udpPort)));
		udpSinkApps.Add (udpSink.Install (nodeDst));

		OnOffHelper onoff ("ns3::UdpSocketFactory", Address (InetSocketAddress (addrDst, udpPort)));
		onoff.SetAttribute ("DataRate", DataRateValue (DataRate ("500kbps")));
		onoff.SetAttribute ("PacketSize", UintegerValue (512));
		udpApps.Add (onoff.Install (nodeSrc));

		NS_LOG (LOG_DEBUG, addrSrc << " => " << addrDst << ":" << udpPort << " [UDP, node " << i << "]");
	}

	// Create the TCP traffic
	NS_LOG (LOG_DEBUG, "Constructing TDP traffic");
	for (size_t i = 1; i < nB.GetN (); ++i) {
		Ptr < Node > nodeSrc = nB.Get (i);
		Ptr < Ipv4 > ipSrc = nodeSrc->GetObject<Ipv4> ();
		Ipv4Address addrSrc = ipSrc->GetAddress (1, 0).GetLocal ();

		Ptr < Node > nodeDst = nD.Get (i);
		Ptr < Ipv4 > ipDst = nodeDst->GetObject<Ipv4> ();
		Ipv4Address addrDst = ipDst->GetAddress (1, 0).GetLocal ();

		OnOffHelper onoff ("ns3::TcpSocketFactory", Address (InetSocketAddress (addrDst, tcpPort)));
		tcpApps.Add (onoff.Install (nodeSrc));

		// Construct the sink end of the TCP flow - install on all just to be safe
		tcpSinkApps.Add (tcpSink.Install (nodeDst));

		NS_LOG (LOG_DEBUG, addrSrc << " => " << addrDst << ":" << tcpPort << " [TCP, node " << i << "]");
	}

	// Start the TCP applications
	udpSinkApps.Start (Seconds (0.0));
	tcpSinkApps.Start (Seconds (0.0));

	udpApps.Start (Seconds (0.0));
	tcpApps.Start (Seconds (0.0));

	udpApps.Stop (Seconds (endTime));
	tcpApps.Stop (Seconds (endTime));

	NS_LOG (LOG_DEBUG, "Enabling global routing");
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	// Log traces across the single link
	linkCore.EnablePcapAll (pcapFn);

	// Run the simulation
	NS_LOG (LOG_INFO, "Starting simulation");
	Simulator::Run ();

	// Get the final simulation runtime
	Time sim_endTime = Simulator::Now ();

	Simulator::Destroy ();

	NS_LOG (LOG_INFO, "Simulation complete");

	// Print out the overall simulation runtime
	std::cout << "Total time: " << Seconds (sim_endTime) << std::endl;

	return 0;
}

