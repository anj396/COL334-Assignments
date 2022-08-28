/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("t1");
int drop=0;
int drop1=0;

// ===========================================================================
//
//         node 1                 node 3                node 2    
//   +----------------+    +----------------+     +----------------+
//   |    ns-3 TCP    |    |    ns-3 TCP    |    |    ns-3 TCP    |
//   +----------------+    +----------------+    +----------------+
//   |    10.1.1.1    |    |    10.1.1.3    |    |    10.1.1.2    |
//   +----------------+    +----------------+    +----------------+
//   | point-to-point |    | point-to-point |    | point-to-point |
//   +----------------+    +----------------+    +----------------+
//           |                     |                     |            
//           +---------------------+---------------------+
//               10 Mbps, 3 ms            9 Mbps, 3 ms
//
// ===========================================================================
// We want to look at changes in the ns-3 TCP congestion window.  We need
// to crank up a flow and hook the CongestionWindow attribute on the socket
// of the sender.  Normally one would use an on-off application to generate a
// flow, but this has a couple of problems.  First, the socket of the on-off
// application is not created until Application Start time, so we wouldn't be
// able to hook the socket (now) at configuration time.  Second, even if we
// could arrange a call after start time, the socket is not public so we
// couldn't get at it.
//
// So, we can cook up a simple version of the on-off application that does what
// we want.  On the plus side we don't need all of the complexity of the on-off
// application.  On the minus side, we don't have a helper, so we have to get
// a little more involved in the details, but this is trivial.
//
// So first, we create a socket and do the trace connect on it; then we pass
// this socket into the constructor of our simple application which we then
// install in the source node.
// ===========================================================================
//
class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyApp")
    .SetParent<Application> ()
    .SetGroupName ("Tutorial")
    .AddConstructor<MyApp> ()
    ;
  return tid;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}

static void
RxDrop (Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
{
  drop++;
  NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
  file->Write (Simulator::Now (), p);
}

static void
RxDrop1 (Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
{
  drop1++;
  NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
  file->Write (Simulator::Now (), p);
}

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  NodeContainer nodes;
  nodes.Create (3);

  PointToPointHelper p0;
  p0.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p0.SetChannelAttribute ("Delay", StringValue ("3ms"));
  
  PointToPointHelper p1;
  p1.SetDeviceAttribute ("DataRate", StringValue ("9Mbps"));
  p1.SetChannelAttribute ("Delay", StringValue ("3ms"));

  NetDeviceContainer d;
  d = p0.Install (nodes.Get(0),nodes.Get(2));
  
  
  NetDeviceContainer d1;
  d1 = p1.Install (nodes.Get(1),nodes.Get(2));
  
  
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
  d.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
  d1.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (d);
  
  
  
  Ipv4AddressHelper address1;
  address1.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces1 = address1.Assign (d1);
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  

  uint16_t sinkPort = 8040;
  Address sinkAddress (InetSocketAddress (interfaces.GetAddress (1), sinkPort));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (2));
  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (20.));
  
  
  uint16_t sinkPort1 = 8070;
  Address sinkAddress1 (InetSocketAddress (interfaces.GetAddress (1), sinkPort1));
  PacketSinkHelper packetSinkHelper1 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort1));
  ApplicationContainer sinkApps1 = packetSinkHelper1.Install (nodes.Get (2));
  sinkApps1.Start (Seconds (4.));
  sinkApps1.Stop (Seconds (25.));
  
  
  uint16_t sinkPort2 = 8100;
  Address sinkAddress2 (InetSocketAddress (interfaces1.GetAddress (1), sinkPort2));
  PacketSinkHelper packetSinkHelper2 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort2));
  ApplicationContainer sinkApps2 = packetSinkHelper2.Install (nodes.Get (2));
  sinkApps2.Start (Seconds (14.));
  sinkApps2.Stop (Seconds (30.));

  
  
  
  
  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());

  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, sinkAddress, 3000, 8000, DataRate ("1.5Mbps"));
  nodes.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (1.));
  app->SetStopTime (Seconds (20.));
  
  Ptr<Socket> ns3TcpSocket1 = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());

  Ptr<MyApp> app1 = CreateObject<MyApp> ();
  app1->Setup (ns3TcpSocket1, sinkAddress1, 3000, 8000, DataRate ("1.5Mbps"));
  nodes.Get (0)->AddApplication (app1);
  app1->SetStartTime (Seconds (5.));
  app1->SetStopTime (Seconds (25.));
  
  Ptr<Socket> ns3TcpSocket2 = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());

  Ptr<MyApp> app2 = CreateObject<MyApp> ();
  app2->Setup (ns3TcpSocket2, sinkAddress2, 3000, 8000, DataRate ("1.5Mbps"));
  nodes.Get (1)->AddApplication (app2);
  app2->SetStartTime (Seconds (15.));
  app2->SetStopTime (Seconds (30.));

  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("t11.txt");
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));


  AsciiTraceHelper asciiTraceHelper1;
  Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper1.CreateFileStream ("t12.txt");
  ns3TcpSocket1->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream1));
  
  AsciiTraceHelper asciiTraceHelper2;
  Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper2.CreateFileStream ("t13.txt");
  ns3TcpSocket2->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream2));


  PcapHelper pcapHelper;
  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("t11.pcap", std::ios::out, PcapHelper::DLT_PPP);
  d.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file));
  
  PcapHelper pcapHelper1;
  Ptr<PcapFileWrapper> file1 = pcapHelper1.CreateFile ("t12.pcap", std::ios::out, PcapHelper::DLT_PPP);
  d1.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop1, file1));

  Simulator::Stop (Seconds (30));
  Simulator::Run ();
  Simulator::Destroy ();
  
  std::cout << "\n Packets Dropped from Node 1 to 3: " << drop << '\n';
  //std::cout << "Packets Dropped from Node 2 to 3: " << drop1<< '\n';

  return 0;
}

