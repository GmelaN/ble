/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 KULeuven 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Stijn Geysen <stijn.geysen@student.kuleuven.be>
 *          Based on the lora ns-3 module written by Brecht Reynders.
 *          This module can be found here:
 *https://github.com/networkedsystems/lora-ns3/blob/master/model/lora-mac-header.h
 
 */


// Include a header file from your module to test.
#include <ns3/log.h>
#include <ns3/core-module.h>
#include <ns3/ble-module.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/packet.h>
#include <ns3/rng-seed-manager.h>
#include <ns3/spectrum-module.h>
#include <ns3/mobility-module.h>
#include <ns3/energy-module.h>
#include <ns3/spectrum-value.h>
#include <ns3/spectrum-analyzer.h>
#include <iostream>
#include <ns3/isotropic-antenna-model.h>
#include <ns3/trace-helper.h>
#include <ns3/drop-tail-queue.h>
#include <unordered_map>
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"


#include <ns3/okumura-hata-propagation-loss-model.h>
//#include "ns3/ble.h"

// An essential include is test.h
#include "ns3/test.h"

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ble-test-broadcast");

// Test case that handles broadcasting.
class BleTestCaseBC : public TestCase
{
public:
  BleTestCaseBC ();
  virtual ~BleTestCaseBC ();

  void Transmitted (const Ptr<const Packet> packet);
  void Received (const Ptr<const Packet> packet);
  void ReceivedBroadcast (
      const Ptr<const Packet> packet, const Ptr<const BleNetDevice> netdevice);
  void ReceivedError (const Ptr<const Packet> packet);
  void ReceivedUnique (const Ptr<const Packet> packet);
  void TXWindowSkipped (const Ptr<const BleNetDevice> nd);

private:
  virtual void DoRun (void);

  /*****************
   * Configuration *
   *****************/

  // Based on Lora battery example

  int nbIterations = 1;
  double length = 30; //<! Square room with length as distance
  int pktsize = 20; //!< Size of packtets, in bytes
  int duration = 110; //<! Duration of the simulation in seconds
  int packetSendDuration = 100; 
    //<! Time during which new packets should be quied 
  bool verbose = false; // Enable logging
  bool nakagami = false; // enable nakagami path loss
  bool dynamic = false; // Wether the nodes are moving yes or no
  bool scheduled = true; // Schedule the TX windows instead of random parameters.
  bool broadcastAvoidCollisions = true; 
  // Try to avoid 2 nodes being in advertising mode at the same time
  uint32_t nNodes = 25; // Number of nodes
  uint32_t nbConnInterval = 3200; 
  // [MAX 3200]  nbConnInterval*1,25ms = size of connection interval. 
  // if nbConnInterval = 0, each link will get a random conn interval
  int interval = 4*nNodes; //!< Time between two packets from the same node 
  // (for good results, should be larger than nNodes*nbConnInterval(s) 
  Ptr<OutputStreamWrapper> m_stream = 0; // Stream for waterfallcurve
  Ptr<UniformRandomVariable> randT = CreateObject<UniformRandomVariable> ();

  std::unordered_map<uint32_t,std::tuple<uint32_t,uint32_t,uint32_t,uint32_t,
    uint32_t,uint32_t,uint32_t,uint32_t> > errorMap;
  //Errormap: transmitted, received, received unique, received original, 
  //xlocation, ylocation
  std::unordered_map<uint32_t,Ptr<BleNetDevice> > deviceMap;

  /************************
   * End of configuration *
   ************************/

};

// Add some help text to this case to describe what it is intended to test
BleTestCaseBC::BleTestCaseBC()
  : TestCase ("Ble test case that handles broadcasting")
{
  //set initial parameter values
}

// This destructor does nothing but we include it as a reminder that
// the test case should clean up after itself
BleTestCaseBC::~BleTestCaseBC ()
{
}
/// Save that the message has been transmitted
	void
BleTestCaseBC::Transmitted (const Ptr<const Packet> packet)
{
	Ptr<Packet> copy = packet->Copy();
	BleMacHeader header;
	copy->RemoveHeader(header);
    uint8_t buffer[2];
    header.GetSrcAddr().CopyTo(buffer);
    uint32_t addr = buffer[1];

	std::get<0>(errorMap[addr-1])++;
}

// save that a message has been received
	void
BleTestCaseBC::Received (const Ptr<const Packet> packet)
{
  //NS_LOG (LOG_DEBUG, "Packet received  " << packet);
	Ptr<Packet> copy = packet->Copy();
	BleMacHeader header;
	copy->RemoveHeader(header);
	uint8_t buffer[2];
    header.GetDestAddr().CopyTo(buffer);
    uint32_t addr = buffer[1];
    std::get<1>(errorMap[addr-1])++;
}

  void
BleTestCaseBC::ReceivedError (const Ptr<const Packet> packet)
{
  //NS_LOG (LOG_DEBUG, "Packet received  " << packet);
	Ptr<Packet> copy = packet->Copy();
	BleMacHeader header;
	copy->RemoveHeader(header);
	uint8_t buffer[2];
    header.GetDestAddr().CopyTo(buffer);
    uint32_t addr = buffer[1];
    std::get<3>(errorMap[addr-1])++;
}


// save that a message has been uniquely received
	void
BleTestCaseBC::ReceivedUnique (const Ptr<const Packet> packet)
{
	Ptr<Packet> copy = packet->Copy();
	BleMacHeader header;
	copy->RemoveHeader(header);
	uint8_t buffer[2];
    header.GetDestAddr().CopyTo(buffer);
    uint32_t addr = buffer[1];
	std::get<2>(errorMap[addr-1])++;
}

	void
BleTestCaseBC::ReceivedBroadcast (const Ptr<const Packet> packet, 
    const Ptr<const BleNetDevice>  netdevice)
{
	Ptr<Packet> copy = packet->Copy();
	BleMacHeader header;
	copy->RemoveHeader(header);
	uint8_t buffer[2];
    netdevice->GetAddress16().CopyTo(buffer);
    uint32_t addr = buffer[1];
	std::get<4>(errorMap[addr-1])++;
}

  void
BleTestCaseBC::TXWindowSkipped (const Ptr<const BleNetDevice> nd)
{
  //NS_LOG (LOG_DEBUG, "Packet received  " << packet);
	uint8_t buffer[2];
    nd->GetAddress16().CopyTo(buffer);
    uint32_t addr = buffer[1];
    std::get<5>(errorMap[addr-1])++;
}



//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
//
void
BleTestCaseBC::DoRun (void)
{
  AsciiTraceHelper ascii;
  m_stream = ascii.CreateFileStream ("databc.csv");
  *m_stream->GetStream() << "#Scenario " << (int)nNodes 
    <<  " nodes on a square field with side " << length << " meter" 
    << " TX window scheduling enabled: " << scheduled 
    << ", connection interval = " << nbConnInterval*1.25 
    << " millisec, (0 = random) " << std::endl;
  // print Iteration, ID, transmitted, received, received unique, 
  // received at closest gateway, x coords, y coords, 
  // get average amount of retransmissions, get average time of transmissions, 
  // number of missed messages, amount of received messages.
  *m_stream->GetStream() << "Iteration, ID, transmitted, received, "
    "received unique, received error, broadcast received, "
    "TX Windows Skipped x coords, y coords " <<std::endl;
  for (uint8_t iterationI=0;iterationI<nbIterations;iterationI++){
		std::cout << "Iteration: " << (int)iterationI << std::endl;
 
    randT->SetAttribute("Max", DoubleValue (600));
    // Enable logging
    BleHelper helper;
    if (verbose)
      helper.EnableLogComponents();


    NS_LOG (LOG_INFO, "Ble Test Case BroadCast setup starts now");

    NodeContainer bleDeviceNodes;
    bleDeviceNodes.Create(nNodes);


    // Create mobility
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> nodePositionList = 
      CreateObject<ListPositionAllocator> ();
    for (uint32_t nodePositionsAssigned = 0; 
        nodePositionsAssigned < nNodes; nodePositionsAssigned++)
    {
      double x,y;
      x = randT->GetInteger(0,length);
      y = randT->GetInteger(0,length);
      NS_LOG (LOG_INFO, "x = " << x << " y = " << y);
      nodePositionList->Add (Vector (x,y,1.0));
    }
    mobility.SetPositionAllocator (nodePositionList);
    if (dynamic)
      mobility.SetMobilityModel ("ns3::RandomWalk2DMobilityModel");
    else
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install(bleDeviceNodes);
    
    // Create the nodes
    NetDeviceContainer bleNetDevices;
    bleNetDevices = helper.Install (bleDeviceNodes);

    // Set addresses
    NS_LOG (LOG_INFO, "Set addresses");
    for (uint32_t nodeI = 0; nodeI < nNodes; nodeI++)
    {
      //std::string s = std::to_string (std::hex(nodeI+1));

      std::stringstream stream;
      stream << std::hex << nodeI+1;
      std::string s( stream.str());
      while (s.size() < 4)
        s.insert(0,1,'0');
      s.insert(2,1,':');
      char const * buffer = s.c_str();
      DynamicCast<BleNetDevice>(bleNetDevices.Get(nodeI))
        ->SetAddress (Mac16Address (buffer));
      NS_LOG (LOG_INFO, "address = " << DynamicCast<BleNetDevice>(
            bleNetDevices.Get(nodeI))->GetAddress ());
    }

    // Create links between the nodes
   //helper.CreateAllLinks (bleNetDevices, scheduled, nbConnInterval);
    helper.CreateBroadcastLink (
        bleNetDevices, scheduled, nbConnInterval, broadcastAvoidCollisions);
   
    NS_LOG (LOG_INFO, " Generate data ");
    ApplicationContainer apps = helper.GenerateBroadcastTraffic (
        randT, bleDeviceNodes, pktsize, 0, packetSendDuration, interval);
    
     // Hookup functions to measure performance

    for (uint32_t i=0; i< bleNetDevices.GetN(); i++)
    {
      uint8_t buffer[2];
      Mac16Address::ConvertFrom(
          bleNetDevices.Get(i)->GetAddress()).CopyTo(buffer);
      uint32_t addr = buffer[1];  
      deviceMap[addr ]=DynamicCast<BleNetDevice>(bleNetDevices.Get(i));
      uint32_t x  = bleNetDevices.Get(i)->GetNode()
        ->GetObject<MobilityModel>()->GetPosition ().x;
      uint32_t y  = bleNetDevices.Get(i)->GetNode()
        ->GetObject<MobilityModel>()->GetPosition ().y;
      errorMap[addr-1] = std::make_tuple (0,0,0,0,0,0,x,y);
      DynamicCast<BleNetDevice>(bleNetDevices.Get(i))
        ->TraceConnectWithoutContext ("MacTx",MakeCallback(
              &BleTestCaseBC::Transmitted, this));
      DynamicCast<BleNetDevice>(bleNetDevices.Get(i))
        ->TraceConnectWithoutContext ("MacRx",MakeCallback(
              &BleTestCaseBC::ReceivedUnique, this));
      DynamicCast<BleNetDevice>(bleNetDevices.Get(i))
        ->TraceConnectWithoutContext ("MacRxBroadcast",MakeCallback(
              &BleTestCaseBC::ReceivedBroadcast, this));
      DynamicCast<BleNetDevice>(bleNetDevices.Get(i))
        ->TraceConnectWithoutContext ("MacPromiscRx",MakeCallback(
              &BleTestCaseBC::Received, this));
      DynamicCast<BleNetDevice>(bleNetDevices.Get(i))
        ->TraceConnectWithoutContext ("MacRxError",MakeCallback(
              &BleTestCaseBC::ReceivedError, this));
      DynamicCast<BleNetDevice>(bleNetDevices.Get(i))
        ->TraceConnectWithoutContext ("TXWindowSkipped",MakeCallback(
              &BleTestCaseBC::TXWindowSkipped, this));
    }

    NS_LOG (LOG_INFO, "Simulator will run now");
    
    Simulator::Stop(Seconds (duration));
    Simulator::Run ();

    for (uint32_t i=0; i< bleNetDevices.GetN(); i++)
    {
            uint8_t buffer[2];
            Mac16Address::ConvertFrom(bleNetDevices.Get(i)
                ->GetAddress()).CopyTo(buffer);
            uint32_t addr = buffer[1];  
            Ptr<BleNetDevice> netdevice =
              DynamicCast<BleNetDevice>(bleNetDevices.Get(i));
            NS_LOG (LOG_DEBUG, "nd = " << netdevice << " addr = " << addr);
			std::tuple<uint32_t,uint32_t,uint32_t,uint32_t,
              uint32_t,uint32_t,uint32_t,uint32_t> tuple = errorMap[i];
			// print iteration, ID, transmitted, received, 
            // received unique, x coords, y coords.
			*m_stream->GetStream() << (int)iterationI << "," 
              << netdevice->GetAddress16() << "," << std::get<0>(tuple)
              << "," << std::get<1>(tuple) << "," <<   std::get<2>(tuple) 
              <<  "," << std::get<3>(tuple) << "," << std::get<4>(tuple) 
              << "," << std::get<5>(tuple)  << "," << std::get<6>(tuple) 
              << "," << std::get<7>(tuple) << std::endl;
		
    }
    errorMap.clear();
    Simulator::Destroy ();
  }
}



// The TestSuite class names the TestSuite, identifies what type of TestSuite,
// and enables the TestCases to be run.  Typically, only the constructor for
// this class must be defined
//
class BleTestSuiteBroadcast : public TestSuite
{
public:
  BleTestSuiteBroadcast ();
};

BleTestSuiteBroadcast::BleTestSuiteBroadcast ()
  : TestSuite ("ble2", Type::UNIT)
{
  // TestDuration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
  AddTestCase (new BleTestCaseBC, Duration::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
static BleTestSuiteBroadcast bleTestSuiteBroadcast;

