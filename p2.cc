/*
 *  ECE 6110
 *  Project 2
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

typedef std::pair<NodeContainer, NetDeviceContainer> topology_t;

PointToPointHelper linkA, linkB, linkCore;

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

topology_t InitNetNodes (const std::vector<size_t>& starSpokes, const size_t spokeNodeCount)
{
	/*
	 * Get a count for the total number of nodes in use.
	 */
	size_t totalNodes = 0;
	for (size_t i = 0; i < starSpokes.size (); ++i)
		totalNodes += starSpokes.at (i);

	/*
	 * Where all nodes are held for the entire topology.
	 */
	NodeContainer nodes;
	NetDeviceContainer devs;

	/*
	 * Create the container used in simulation for
	 * representing the computers.
	 */
	NS_LOG (LOG_DEBUG, "Creating " << starSpokes.size () << " CSMA stars");

	/*
	 * Iterate over all star net devices.
	 */
	for (size_t i = 0; i < starSpokes.size (); ++i) {
		const size_t numSpokes = starSpokes.at (i);
		NS_LOG (LOG_DEBUG,
				"Creating a " << numSpokes << " spoke CSMA star with " << spokeNodeCount << " nodes on each spoke");

		/*
		 * Create a CSMA Star net device.
		 */
		CsmaHelper csma;
		csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
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
			nn.Create (spokeNodeCount);

			/*
			 * Add the spoke to the overall node container.
			 */
			nodes.Add (nn);

			NS_LOG (LOG_DEBUG, "Currently " << nodes.GetN () << " total nodes");

			devs.Add (csma.Install (nn, csmaChannel));
		}

		// Add the hub node itself to the list
		nodes.Add (star.GetHub ());
	}

	/*
	 * Create point-to-point links connecting all of the stars together.
	 */
	NS_LOG (LOG_DEBUG, "Creating nodes for middle dumbbell link");
	NodeContainer nn;
	nn.Create (2);
	// Add them to the nodes list
	nodes.Add (nn);

	// Represents the index of the hub nodes for each star CSMA LAN
	size_t hub1_1 = spokeNodeCount * starSpokes.at (0);
	size_t hub1_2 = hub1_1 + (spokeNodeCount * starSpokes.at (1));
	size_t hub2_1 = hub1_2 + (spokeNodeCount * starSpokes.at (2));
	size_t hub2_2 = hub2_1 + (spokeNodeCount * starSpokes.at (3));

	/*
	 * Set the attributes for the links between the star subnets and
	 * for the dumbbell link.
	 */
	linkA.SetQueue ("ns3::DropTailQueue");
	linkB.SetQueue ("ns3::DropTailQueue");
	linkCore.SetQueue ("ns3::DropTailQueue");
//	linkCore.SetQueue ("ns3::RedQueue");
//	linkA.SetQueue ("ns3::RedQueue");
//	linkB.SetQueue ("ns3::RedQueue");

	linkA.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	linkA.SetChannelAttribute ("Delay", StringValue ("15ms"));

	linkB.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	linkB.SetChannelAttribute ("Delay", StringValue ("15ms"));

	linkCore.SetDeviceAttribute ("DataRate", StringValue ("3Mbps"));
	linkCore.SetChannelAttribute ("Delay", StringValue ("5ms"));

	/*
	 * Create the network devices.
	 */
	NS_LOG (LOG_DEBUG, "Creating network interface devices for " << nodes.GetN () << " nodes");

	// Index values for left and right nodes
	const size_t leftNode = nodes.GetN () - 1;
	const size_t rightNode = nodes.GetN () - 2;

	NS_LOG (LOG_DEBUG, "Connecting node " << hub1_1 << " to the dumbbell's left side");
	devs.Add (linkA.Install (nodes.Get (hub1_1), nodes.Get (leftNode)));
	NS_LOG (LOG_DEBUG, "Connecting node " << hub1_2 << " to the dumbbell's left side");
	devs.Add (linkA.Install (nodes.Get (hub1_2), nodes.Get (leftNode)));

	NS_LOG (LOG_DEBUG, "Connecting node " << hub2_1 << " to the dumbbell's right side");
	devs.Add (linkB.Install (nodes.Get (hub2_1), nodes.Get (rightNode)));
	NS_LOG (LOG_DEBUG, "Connecting node " << hub2_2 << " to the dumbbell's right side");
	devs.Add (linkB.Install (nodes.Get (hub2_2), nodes.Get (rightNode)));

	NS_LOG (LOG_DEBUG, "Connecting the two sides together through the dumbbell link");
	devs.Add (linkCore.Install (nodes.Get (leftNode), nodes.Get (rightNode)));

	// Wrap up the topology devices and nodes and return them
	topology_t topo;
	topo.first = nodes;
	topo.second = devs;
	return topo;
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
	size_t nFlows = 1;
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

	/*
	 * Vector representing the number of subnodes connected
	 * to each subnode.
	 */
	size_t sss[] = { 4, 4, 4, 4 };
	std::vector < size_t > starSpokes (begin (sss), end (sss));

	size_t nodesPerSpoke = 4;
	topology_t topo = InitNetNodes (starSpokes, nodesPerSpoke);

	// These hold all devices and nodes in the topology
	NodeContainer nodes;
	NetDeviceContainer devs;
	nodes = topo.first;
	devs = topo.second;

	// Set IPv4, IPv6, UDP, & TCP stacks to all nodes in the simulation
	NS_LOG (LOG_DEBUG, "Setting simulation to use IPv4, IPv6, UDP, & TCP stacks");
	InternetStackHelper stack;
	stack.InstallAll ();

	NS_LOG (LOG_DEBUG, "Assigning IPv4 addresses for " << devs.GetN () << " devices");
	Ipv4AddressHelper addrsA, addrsB, addrsC, addrsD;

	// Define the addresses
	addrsA.SetBase ("10.4.0.0", "255.255.255.0");
	addrsB.SetBase ("10.8.0.0", "255.255.255.0");
	addrsC.SetBase ("10.4.64.0", "255.255.255.0");
	addrsD.SetBase ("10.8.64.0", "255.255.255.0");

	std::vector < Ipv4AddressHelper > addrs;
	addrs.push_back (addrsA);
	addrs.push_back (addrsB);
	addrs.push_back (addrsC);
	addrs.push_back (addrsD);

	// Assign the addresses to devices
	Ipv4InterfaceContainer nics;
	for (size_t i = 0; i < addrs.size (); ++i) {
		// helper for assigning addresses to the net devices
		Ipv4AddressHelper addrsSet (addrs.at (i));

		const size_t numSpoke = starSpokes.at (i);
		const size_t numNodes = numSpoke * nodesPerSpoke;
		const size_t startNode = i * numNodes;
		for (size_t j = startNode; j < startNode + numNodes; ++j) {
			nics.Add (addrsSet.Assign (devs.Get (j)));
			NS_LOG (LOG_DEBUG, "Assigned " << nics.GetAddress (j) << " to device " << j);
		}
	}

	/*
	 * Assign addresses to the connecting point-to-point links.
	 */
	NS_LOG (LOG_DEBUG, "Assigning addresses to dumbbell link devices");

	Ipv4AddressHelper addrsLeft, addrsRight, addrsCore;
	addrsCore.SetBase ("8.6.4.0", "255.255.255.192");
	addrsLeft.SetBase ("57.91.0.0", "255.255.255.192");
	addrsRight.SetBase ("75.15.0.0", "255.255.255.192");

	for (size_t i = 0; i < 4; ++i) {
		const size_t ii = devs.GetN () + i - 10;
		nics.Add (addrsLeft.Assign (devs.Get (ii)));
		NS_LOG (LOG_DEBUG, "Assigned " << nics.GetAddress (ii) << " to device " << ii);
	}

	for (size_t i = 0; i < 4; ++i) {
		const size_t ii = devs.GetN () + i - 6;
		nics.Add (addrsRight.Assign (devs.Get (ii)));
		NS_LOG (LOG_DEBUG, "Assigned " << nics.GetAddress (ii) << " to device " << ii);
	}

	for (size_t i = 0; i < 2; ++i) {
		const size_t ii = devs.GetN () + i - 2;
		nics.Add (addrsCore.Assign (devs.Get (ii)));
		NS_LOG (LOG_DEBUG, "Assigned " << nics.GetAddress (ii) << " to device " << ii);
	}

	NS_LOG (LOG_DEBUG, "Enabling global routing");
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	NS_LOG (LOG_DEBUG, nics.GetN () << " total interfaces");
	NS_LOG (LOG_DEBUG, nodes.GetN () << " total nodes");

	// Create traffic sending applications
	ApplicationContainer udpApps, tcpApps, udpSinkApps, tcpSinkApps;

	typedef std::pair<size_t, size_t> nodePair_t;
	std::vector<nodePair_t> udpConvs, tcpConvs;

	// Create pairs of nodes that will send data back and forwarth
//	const size_t nodesPerLan = nodesPerSpoke * starSpokes.at (0);
	const uint16_t uspPort = 9, tcpPort = 8080;

	PacketSinkHelper udpSink ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), uspPort));
	PacketSinkHelper tcpSink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), tcpPort));

	// Star LAN numbers 0 and 2 will be sending UDP data across the queues
//	for (size_t i = 0; i < nodesPerLan; ++i)
	for (size_t i = 0; i < starSpokes.size(); ++i) {
		const nodesPerLan = nodesPerStoke * starSpokes.at(i);
		udpConvs.push_back (std::make_pair ((0 * nodesPerLan) + i, (2 * nodesPerLan) + i));
	}

	for (size_t i = 0; i < udpConvs.size (); ++i) {
		nodePair_t c = udpConvs.at (i);

		OnOffHelper onoff ("ns3::UdpSocketFactory", Address (InetSocketAddress (nics.GetAddress (c.first), uspPort)));
//		onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
//		onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

		udpApps.Add (onoff.Install (nodes.Get (c.first)));

		// Construct the sink end of the TCP flow - install on all just to be safe
		udpSinkApps.Add (udpSink.Install (nodes.Get (c.second + 2)));

		NS_LOG (LOG_DEBUG,
				nics.GetAddress (c.first) << " (node " << c.first << ") will send random UDP traffic to "
						<< nics.GetAddress (c.second) << ":" << udp_port << " (node " << c.second << ")");
	}

	// Start the UDP applications
	udpSinkApps.Start (Seconds (0.0));
	udpApps.Start (Seconds (0.0));

	// Star LAN numbers 1 and 3 will be sending TCP data across the queues
	for (size_t i = 0; i < nodesPerLan; ++i)
		tcpConvs.push_back (std::make_pair ((1 * nodesPerLan) + i, (3 * nodesPerLan) + i));

	for (size_t i = 0; i < tcpConvs.size (); ++i) {
		nodePair_t c = tcpConvs.at (i);

		OnOffHelper onoff ("ns3::TcpSocketFactory", Address (InetSocketAddress (nics.GetAddress (c.second), tcpPort)));
//		onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
//		onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

		tcpApps.Add (onoff.Install (nodes.Get (c.first + 1)));

		// Construct the sink end of the TCP flow - install on all just to be safe
		tcpSinkApps.Add (tcpSink.Install (nodes.Get (c.second + 3)));

		NS_LOG (LOG_DEBUG,
				nics.GetAddress (c.first) << " (node " << c.first << ") will send random TCP traffic to "
						<< nics.GetAddress (c.second) << ":" << tcp_port << " (node " << c.second << ")");
	}

	// Start the TCP applications
	tcpSinkApps.Start (Seconds (0.0));
	tcpApps.Start (Seconds (0.0));

	const double endTime = 1000.0;
	udpApps.Stop (Seconds (endTime));
	tcpApps.Stop (Seconds (endTime));

	linkCore.EnablePcapAll ("linkCore");

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

