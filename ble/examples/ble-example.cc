/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/*
 * Information from:
 *  Sixlowpan - example-sixlowpan.cc
 */

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/ble-helper.h"
#include "ns3/ble-module.h"

#include "ns3/internet-module.h"
//#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/sixlowpan-module.h"

#include "ns3/ping6-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("BleExample");

int main (int argc, char** argv)
{
  bool verbose = false;

  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell application to log if true", verbose);

  cmd.Parse (argc,argv);

  // Logging
  if (verbose)
    {
      LogComponentEnable ("SixLowPanNetDevice", LOG_LEVEL_ALL);
      LogComponentEnable ("Ping6Application", LOG_LEVEL_ALL);
    }

  Packet::EnablePrinting ();
  Packet::EnableChecking ();

  NS_LOG_INFO ("BLE example file");

  // Create Nodes
  NS_LOG_INFO ("Create nodes.");
  Ptr<Node> n0 = CreateObject<Node> ();
  Ptr<Node> r = CreateObject<Node> ();
  Ptr<Node> n1 = CreateObject<Node> ();

  NodeContainer net1 (n0, r);
  NodeContainer net2 (r, n1);
  NodeContainer all (n0, r, n1);

  // Create IPv6 stack
  NS_LOG_INFO ("Create IPv6 Internet Stack");
  InternetStackHelper internetv6;
  internetv6.Install (all);

  // Create BLE (CSMA) channels
  /* This part should be changed to BLE */ 
  NS_LOG_INFO ("Create channels.");
  BleHelper ble;
  ble.EnableLogComponents();
  // TODO?
  //ble.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  //ble.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
//  NetDeviceContainer d2 = ble.Install (net2);
//  ble.SetDeviceAttribute ("Mtu", UintegerValue (150));
//  NetDeviceContainer d1 = ble.Install (net1);

//  CsmaHelper csma;
//  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
//  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
//  NetDeviceContainer d2 = csma.Install (net2);
//  csma.SetDeviceAttribute ("Mtu", UintegerValue (150));
//  NetDeviceContainer d1 = csma.Install (net1);

//  SixLowPanHelper sixlowpan;
//  sixlowpan.SetDeviceAttribute ("ForceEtherType", BooleanValue (true) );
//  NetDeviceContainer six1 = sixlowpan.Install (d1);
//
//  // Create Networks
//  NS_LOG_INFO ("Create networks and assign IPv6 Addresses.");
//  Ipv6AddressHelper ipv6;
//  ipv6.SetBase (Ipv6Address ("2001:1::"), Ipv6Prefix (64));
//  Ipv6InterfaceContainer i1 = ipv6.Assign (six1);
//  i1.SetForwarding (1, true);
//  i1.SetDefaultRouteInAllNodes (1);
//
//  ipv6.SetBase (Ipv6Address ("2001:2::"), Ipv6Prefix (64));
//  Ipv6InterfaceContainer i2 = ipv6.Assign (d2);
//  i2.SetForwarding (0, true);
//  i2.SetDefaultRouteInAllNodes (0);

  // Create Application
  /* Create a Ping6 application to send ICMPv6 echo request from n0 to n1 via r */
  NS_LOG_INFO ("Create Application ping6.");
 // uint32_t packetSize = 200;
 // uint32_t maxPacketCount = 50;
 // Time interPacketInterval = Seconds (1.);
 // Ping6Helper ping6;

 // ping6.SetLocal (i1.GetAddress (0, 1));
 // ping6.SetRemote (i2.GetAddress (1, 1));

 // ping6.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
 // ping6.SetAttribute ("Interval", TimeValue (interPacketInterval));
 // ping6.SetAttribute ("PacketSize", UintegerValue (packetSize));
 // ApplicationContainer apps = ping6.Install (net1.Get (0));

 // apps.Start (Seconds (5.0));
 // apps.Stop (Seconds (15.0));

  // Enable debug output
  /* Csma has to be changed to BLE */ 
  NS_LOG_INFO ("Enable debug output");
  AsciiTraceHelper ascii;
//  csma.EnableAsciiAll (ascii.CreateFileStream ("example-ble.tr"));
//  csma.EnablePcapAll (std::string ("example-ble"), true);
  ble.EnableAsciiAll (ascii.CreateFileStream ("example-ble.tr"));
  
  //ble.EnablePcapAll (std::string ("example-ble"), true);


  // Simulation details
  Simulator::Stop (Seconds (100));
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
  return 0;
}


