/*
 *  ECE 6110
 *  Project 2
 *
 *  Jonathan Jones
 *
 *  Topology shown in results pdf.
 *
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

void RttCallback(std::string context, Ptr<const Packet> p, const Address& address) {

}

int main (int argc, char* argv[])
{
	// Parse any command line arguments
	CommandLine cmd;
	std::string queueType("DropTailQueue");
	bool traceEN = true;
	std::string pcapFn ("queue-trace-results");
	double endTime = 20.0;
	size_t rttMultiplier = 1;
	size_t datarateMultiplier = 1;
	std::string xmlFn ("/home/jonathan/Documents/queue-sims/ns3-config1.xml");

	cmd.AddValue ("queueType", "The type of queue to use for the run ('DropTailQueue' or 'RedQueue')", queueType);
	cmd.AddValue ("trace", "Enable/Disable dumping the trace at the TCP sink", traceEN);
	cmd.AddValue ("traceFile", "Base name given to where the results are saved when enabled", pcapFn);
	cmd.AddValue ("endTime", "The duration when the simulation should be stopped", endTime);
	cmd.AddValue ("rttMultiplier", "An integer multiplier for the round trip time of the topology", rttMultiplier);
	cmd.AddValue ("datarateMultiplier", "An integer multiplier for the datarate.", datarateMultiplier);
	cmd.AddValue ("xml", "The name for the XML configuration file.", xmlFn);
	cmd.Parse (argc, argv);

	// Prefix the queue type with the proper namespace declaration for ns-3
	queueType = "ns3::" + queueType;

	// Initial simulation configurations
	SetSimConfigs (xmlFn);

	/*
	 * Where all the endpoint nodes in the topology are held.
	 */
	NodeContainer nCore;
	NodeContainer nA, nB, nC, nD;
	NodeContainer n_A, n_B, n_C, n_D;


	NS_LOG (LOG_DEBUG, "Creating nodes for middle dumbbell link");
	nCore.Create (2);

	/**
	 * Place 2 nodes onto each of the 4 corners of the topology
	 */
	NS_LOG (LOG_DEBUG, "Creating nodes for lower sublinks");
	nA.Create(2);
	nB.Create(2);
	nC.Create(2);
	nD.Create(2);

	/**
	 * Create node groups for links leading to the center dumbbell nodes
	 */
	NS_LOG (LOG_DEBUG, "Creating nodes for linking sublinks to the core nodes");
	n_A = NodeContainer (nA.Get (0), nCore.Get (0));
	n_B = NodeContainer (nB.Get (0), nCore.Get (0));
	n_C = NodeContainer (nC.Get (0), nCore.Get (1));
	n_D = NodeContainer (nD.Get (0), nCore.Get (1));

	// Links between the lower subnodes
	PointToPointHelper linkA1_A2, linkB1_B2, linkC1_C2, linkD1_D2;
	// Links leading to the core nodes
	PointToPointHelper linkA_Core1, linkB_Core1, linkC_Core2, linkD_Core2;
	// Link between the core nodes - the dumbbell link
	PointToPointHelper linkCore1_Core2;

	/*
	 * Set the attributes for the links between the star subnets and
	 * for the dumbbell link.
	 */
	linkA1_A2.SetQueue (queueType);
	linkB1_B2.SetQueue (queueType);
	linkC1_C2.SetQueue (queueType);
	linkD1_D2.SetQueue (queueType);
	linkA_Core1.SetQueue (queueType);
	linkB_Core1.SetQueue (queueType);
	linkC_Core2.SetQueue (queueType);
	linkD_Core2.SetQueue (queueType);
	linkCore1_Core2.SetQueue (queueType);

	/**
	 * Set the round trip times based on the multiplier value
	 */
	std::ostringstream conv;
	conv << rttMultiplier * 3;
	std::string rtt1(conv.str() + "ms");
	NS_LOG (LOG_DEBUG, "RTT 1:\t" << rtt1);

	conv.str(""); conv.clear();
	conv << rttMultiplier * 7;
	std::string rtt2(conv.str() + "ms");
	NS_LOG (LOG_DEBUG, "RTT 1:\t" << rtt2);

	conv.str(""); conv.clear();
	conv << rttMultiplier * 12;
	std::string rtt3(conv.str() + "ms");
	NS_LOG (LOG_DEBUG, "RTT 1:\t" << rtt3);

	linkA1_A2.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	linkA1_A2.SetChannelAttribute ("Delay", StringValue (rtt1));
	linkB1_B2.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	linkB1_B2.SetChannelAttribute ("Delay", StringValue (rtt1));
	linkC1_C2.SetDeviceAttribute ("DataRate", StringValue ("5bps"));
	linkC1_C2.SetChannelAttribute ("Delay", StringValue (rtt1));
	linkD1_D2.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	linkD1_D2.SetChannelAttribute ("Delay", StringValue (rtt1));

	linkA_Core1.SetDeviceAttribute ("DataRate", StringValue ("3Mbps"));
	linkA_Core1.SetChannelAttribute ("Delay", StringValue (rtt2));
	linkB_Core1.SetDeviceAttribute ("DataRate", StringValue ("3Mbps"));
	linkB_Core1.SetChannelAttribute ("Delay", StringValue (rtt2));
	linkC_Core2.SetDeviceAttribute ("DataRate", StringValue ("3Mbps"));
	linkC_Core2.SetChannelAttribute ("Delay", StringValue (rtt2));
	linkD_Core2.SetDeviceAttribute ("DataRate", StringValue ("3Mbps"));
	linkD_Core2.SetChannelAttribute ("Delay", StringValue (rtt2));

	linkCore1_Core2.SetDeviceAttribute ("DataRate", StringValue ("3Mbps"));
	linkCore1_Core2.SetChannelAttribute ("Delay", StringValue (rtt3));

	/*
	 * Create the network devices.
	 */
	NetDeviceContainer dCore;
	NetDeviceContainer dA, dB, dC, dD;
	NetDeviceContainer d_A, d_B, d_C, d_D;

	NS_LOG (LOG_DEBUG, "Connecting lower nodes together");
	dA.Add (linkA1_A2.Install (nA));
	dB.Add (linkB1_B2.Install (nB));
	dC.Add (linkC1_C2.Install (nC));
	dD.Add (linkD1_D2.Install (nD));

	NS_LOG (LOG_DEBUG, "Connecting lower nodes to core");
	d_A.Add (linkA_Core1.Install (n_A));
	d_B.Add (linkA_Core1.Install (n_B));
	d_C.Add (linkC_Core2.Install (n_C));
	d_D.Add (linkD_Core2.Install (n_D));

	NS_LOG (LOG_DEBUG, "Connecting core nodes together");
	dCore.Add (linkCore1_Core2.Install (nCore));

	// Set IPv4, IPv6, UDP, & TCP stacks to all nodes in the simulation
	NS_LOG (LOG_DEBUG, "Setting simulation to use IPv4, IPv6, UDP, & TCP stacks");
	InternetStackHelper stack;
	stack.InstallAll ();

	Ipv4AddressHelper ipv4;
	NS_LOG (LOG_DEBUG, "Assigning IPv4 addresses for lower links");
	ipv4.SetBase ("10.0.0.0", "255.255.255.0");
	ipv4.Assign (dA);
	ipv4.SetBase ("10.0.1.0", "255.255.255.0");
	ipv4.Assign (dB);
	ipv4.SetBase ("10.0.2.0", "255.255.255.0");
	ipv4.Assign (dC);
	ipv4.SetBase ("10.0.3.0", "255.255.255.0");
	ipv4.Assign (dD);

	NS_LOG (LOG_DEBUG, "Assigning IPv4 addresses for links to the core");
	ipv4.SetBase ("172.16.0.0", "255.255.255.0");
	ipv4.Assign (d_A);
	ipv4.SetBase ("172.17.0.0", "255.255.255.0");
	ipv4.Assign (d_B);
	ipv4.SetBase ("172.18.0.0", "255.255.255.0");
	ipv4.Assign (d_C);
	ipv4.SetBase ("172.19.0.0", "255.255.255.0");
	ipv4.Assign (d_D);

	NS_LOG (LOG_DEBUG, "Assigning IPv4 addresses for the core link");
	ipv4.SetBase ("57.20.43.0", "255.255.255.0");
	ipv4.Assign (dCore);

	// Create traffic sending applications
	ApplicationContainer udpApps, tcpApps, udpSinkApps, tcpSinkApps;
	const uint16_t udpPort = 9, tcpPort = 8080;

	// convert the datarates into string values
	conv.str(""); conv.clear();
	conv << datarateMultiplier * 0.58;
	std::string datarate1_3(conv.str() + "Mbps");
	NS_LOG (LOG_DEBUG, "DataRate 1 & 3:\t" << datarate1_3);

	conv.str(""); conv.clear();
	conv << datarateMultiplier * 0.33;
	std::string datarate2(conv.str() + "Mbps");
	NS_LOG (LOG_DEBUG, "DataRate 2:\t" << datarate2);

	// ===== Create the UDP traffic =====
	NS_LOG (LOG_DEBUG, "Constructing UDP traffic");
	// Setup a UDP flow going from the bottom of A to the bottom of C
	Ptr < Node > nUdpSrc1 = nA.Get (1);
	Ptr < Node > nUdpDst1 = nC.Get (1);
	Ptr < Ipv4 > updSrc1 = nUdpSrc1->GetObject<Ipv4> ();
	Ptr < Ipv4 > udpDst1 = nUdpDst1->GetObject<Ipv4> ();
	Ipv4Address udpSrcAddr1 = updSrc1->GetAddress (1, 0).GetLocal ();
	Ipv4Address udpDstAddr1 = udpDst1->GetAddress (1, 0).GetLocal ();
	PacketSinkHelper udpSink1 ("ns3::UdpSocketFactory", Address (InetSocketAddress (udpDstAddr1, udpPort)));
	udpSinkApps.Add (udpSink1.Install (nUdpDst1));
	OnOffHelper onOff_Udp1 ("ns3::UdpSocketFactory", Address (InetSocketAddress (udpDstAddr1, udpPort)));
	onOff_Udp1.SetAttribute ("OnTime", StringValue ("ns3::UniformRandomVariable[Min=0.,Max=1.]"));
	onOff_Udp1.SetAttribute ("OffTime", StringValue ("ns3::UniformRandomVariable[Min=0.,Max=1.]"));
	onOff_Udp1.SetAttribute ("DataRate", DataRateValue (DataRate (datarate1_3)));
	udpApps.Add (onOff_Udp1.Install (nUdpSrc1));
	NS_LOG (LOG_DEBUG, udpSrcAddr1 << " => " << udpDstAddr1 << ":" << udpPort << " [UDP]");

	// Setup a UDP flow going from the bottom of D to the bottom of C
	Ptr < Node > nUdpSrc2 = nD.Get (1);
	Ptr < Node > nUdpDst2 = nC.Get (1);
	Ptr < Ipv4 > updSrc2 = nUdpSrc2->GetObject<Ipv4> ();
	Ptr < Ipv4 > udpDst2 = nUdpDst2->GetObject<Ipv4> ();
	Ipv4Address udpSrcAddr2 = updSrc2->GetAddress (1, 0).GetLocal ();
	Ipv4Address udpDstAddr2 = udpDst2->GetAddress (1, 0).GetLocal ();
	PacketSinkHelper udpSink2 ("ns3::UdpSocketFactory", Address (InetSocketAddress (udpDstAddr2, 2 * udpPort)));
	udpSinkApps.Add (udpSink2.Install (nUdpDst2));
	OnOffHelper onOff_Udp2 ("ns3::UdpSocketFactory", Address (InetSocketAddress (udpDstAddr2, 2 * udpPort)));
	onOff_Udp2.SetAttribute ("OnTime", StringValue ("ns3::UniformRandomVariable[Min=0.,Max=1.]"));
	onOff_Udp2.SetAttribute ("OffTime", StringValue ("ns3::UniformRandomVariable[Min=0.,Max=1.]"));
	onOff_Udp2.SetAttribute ("DataRate", DataRateValue (DataRate (datarate2)));
	udpApps.Add (onOff_Udp2.Install (nUdpSrc2));
	NS_LOG (LOG_DEBUG, udpSrcAddr2 << " => " << udpDstAddr2 << ":" << 2 * udpPort << " [UDP]");

	// ===== Create the TCP traffic =====
	NS_LOG (LOG_DEBUG, "Constructing TDP traffic");
	PacketSinkHelper tcpSink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), tcpPort));
	// Setup a UDP flow going from the bottom of B to the bottom of D
	Ptr < Node > nTcpSrc1 = nB.Get (1);
	Ptr < Node > nTcpDst1 = nD.Get (1);
	Ptr < Ipv4 > tcpSrc1 = nTcpSrc1->GetObject<Ipv4> ();
	Ptr < Ipv4 > tcpDst1 = nTcpDst1->GetObject<Ipv4> ();
	Ipv4Address tcpSrcAddr1 = tcpSrc1->GetAddress (1, 0).GetLocal ();
	Ipv4Address tcpDstAddr1 = tcpDst1->GetAddress (1, 0).GetLocal ();
	OnOffHelper onOff_Tcp1 ("ns3::TcpSocketFactory", Address (InetSocketAddress (tcpDstAddr1, tcpPort)));
	onOff_Tcp1.SetAttribute ("OnTime", StringValue ("ns3::UniformRandomVariable[Min=0.,Max=1.]"));
	onOff_Tcp1.SetAttribute ("OffTime", StringValue ("ns3::UniformRandomVariable[Min=0.,Max=1.]"));
	onOff_Tcp1.SetAttribute ("DataRate", DataRateValue (DataRate (datarate1_3)));
	tcpApps.Add (onOff_Tcp1.Install (nTcpSrc1));
	tcpSinkApps.Add (tcpSink.Install (nTcpDst1));
	NS_LOG (LOG_DEBUG, tcpSrcAddr1 << " => " << tcpDstAddr1 << ":" << tcpPort << " [TCP]");

	// Start the applications at time 0
	udpApps.Start (Seconds (0.0));
	udpSinkApps.Start (Seconds (0.0));
	tcpApps.Start (Seconds (0.0));
	tcpSinkApps.Start (Seconds (0.0));

	// Stop at the given endtime
	udpApps.Stop (Seconds (endTime));
	tcpApps.Stop (Seconds (endTime));

	NS_LOG (LOG_DEBUG, "Enabling global routing");
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	// Log traces across the single link
	if (traceEN)
		linkCore1_Core2.EnablePcapAll (pcapFn);

	// Run the simulation
	NS_LOG (LOG_INFO, "Starting simulation");
	Simulator::Run ();

	std::cout << "Queue Type:\t" << queueType << std::endl;

	uint32_t totalRxBytesTcp = 0;
	for (uint32_t i = 0; i < tcpSinkApps.GetN (); i++) {
		Ptr <Application> app = tcpSinkApps.Get (i);
		Ptr <PacketSink> pktSink = DynamicCast <PacketSink> (app);
		totalRxBytesTcp += pktSink->GetTotalRx ();
	}

	std::cout << "TCP Goodput:\t" << totalRxBytesTcp / Simulator::Now ().GetSeconds () << " bytes/sec." << std::endl;

	// Sum up the number of received bytes
	uint32_t totalRxBytesUdp = 0;
	for (uint32_t i = 0; i < udpSinkApps.GetN (); i++) {
		Ptr <Application> app = udpSinkApps.Get (i);
		Ptr <PacketSink> pktSink = DynamicCast <PacketSink> (app);
		totalRxBytesUdp += pktSink->GetTotalRx ();
	}

	std::cout << "UDP Goodput:\t" << totalRxBytesUdp / Simulator::Now ().GetSeconds () << " bytes/sec." << std::endl;

	// Print out the overall simulation runtime
	std::cout << "Total Time:\t" << Simulator::Now ().GetSeconds () << " s" << std::endl;

	NS_LOG (LOG_INFO, "Simulation complete, destroying simulator");
	Simulator::Destroy ();

	return 0;
}

