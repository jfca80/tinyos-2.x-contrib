/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.  By
 * downloading, copying, installing or using the software you agree to
 * this license.  If you do not agree to this license, do not download,
 * install, copy or use the software.
 *
 * Copyright (c) 2006-2008 Vrije Universiteit Amsterdam and
 * Development Laboratories (DevLab), Eindhoven, the Netherlands.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions, the author, and the following
 *   disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions, the author, and the following disclaimer
 *   in the documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the Vrije Universiteit Amsterdam, nor the name of
 *   DevLab, nor the names of their contributors may be used to endorse or
 *   promote products derived from this software without specific prior
 *   written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL VRIJE
 * UNIVERSITEIT AMSTERDAM, DEVLAB, OR THEIR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Konrad Iwanicki
 * CVS id: $Id$
 */
#include "ClusterHierarchy.h"


/**
 * An engine for maintaining a recursive cluster hierarchy that uses the
 * hybrid algorithm, combining periodic hierarchical distance-vector
 * algorithm and reactive beaconing to speed up hierarchy convergence.
 *
 * More information on such protocols can be found in:
 * K. Iwanicki and M. van Steen, ``On Hierarchical Routing in Wireless
 * Sensor Networks.'' In <i>IPSN'09: Proceedings of the Eighth ACM/IEEE
 * International Conference on Information Processing in Sensor Networks</i>,
 * San Francisco, California, USA, April 13-16, 2009.
 *
 * @author Konrad Iwanicki &lt;iwanicki@few.vu.nl&gt;
 */
configuration HybridBeaconingAndPeriodicDistanceVectorC {
    provides {
        interface Init;
        interface ClusterHierarchyMaintenanceConfig as Config;
        interface ClusterHierarchyMaintenanceControl as Control;

        interface ClusterHierarchy;
        interface NxVector<am_addr_t> as MsgLabel;

        interface TOSSIMStats;
    }
    uses {
        interface NeighborTable as NeighborTable;
        interface Iterator<neighbor_iter_t, neighbor_t>
            as NeighborTableIter;

        interface Packet as HeartbeatPacket;
        interface AMPacket as HeartbeatAMPacket;
        interface Receive as HeartbeatReceive;
        interface AMSend as HeartbeatSend;

        interface Packet as BeaconPacket;
        interface AMPacket as BeaconAMPacket;
        interface Receive as BeaconReceive;
        interface AMSend as BeaconSend;

        interface PoolOfMessages as MessagePool;

        interface Random as Random;
    }
}
implementation {

    components HybridBeaconingAndPeriodicDistanceVectorP as CHEngineP;

    components CHConfigP;
    components CHLocalStateP;
    components CHClusteringP;
    components CHRoutingP;

    components new MessagePumpP(CH_MAX_LABEL_LENGTH, 0) as CHMessagePumpP;

#ifdef CH_NON_COMPRESSED_MSGS
    components new NxVectorGenericC(
        uint16_t, CH_MAX_LABEL_LOGLENGTH, (sizeof(uint16_t) << 3)) as CHLabelP;
    components new NxVectorGenericC(
        ch_phdv_ucnt_t, CH_MAX_LABEL_LOGLENGTH, (sizeof(ch_phdv_ucnt_t) << 3)) as CHLabelUpdateVectP;
#else
    components new NxVectorGenericC(
        uint16_t, CH_MAX_LABEL_LOGLENGTH, 10) as CHLabelP;
    components new NxVectorGenericC(
        ch_phdv_ucnt_t, CH_MAX_LABEL_LOGLENGTH, 10) as CHLabelUpdateVectP;
#endif //CH_NON_COMPRESSED_MSGS
    components CHMsgRTEntry4BitLevel11BitHeadP as CHRTEntryP;

    components new TimerMilliC() as CHMainTimerP;
    components new TimerMilliC() as CHMessagePumpTimerP;
    components new TimerMilliC() as CHPromotionTimerP;



    // ------------------------ Provided interfaces -----------------------
    Init = CHEngineP.Init;
    Config = CHConfigP.Config;
    Control = CHEngineP.Control;
    ClusterHierarchy = CHEngineP.ClusterHierarchy;
    MsgLabel = CHLabelP;
    TOSSIMStats = CHEngineP.TOSSIMStats;


    // -------------------------- Used interfaces -------------------------
    CHEngineP.Config -> CHConfigP;
    CHEngineP.SubMsgLabel -> CHLabelP;
    CHEngineP.SubMsgUpdateVect -> CHLabelUpdateVectP;
    CHEngineP.SubMsgRTEntry -> CHRTEntryP;

    CHEngineP.NeighborTable = NeighborTable;
    CHEngineP.NeighborTableIter = NeighborTableIter;

    CHEngineP.MainTimer -> CHMainTimerP;
    CHEngineP.PromotionTimer -> CHPromotionTimerP;

    CHEngineP.HeartbeatSend = HeartbeatSend;
    CHEngineP.HeartbeatPacket = HeartbeatPacket;
    CHEngineP.HeartbeatAMPacket = HeartbeatAMPacket;
    CHEngineP.HeartbeatReceive = HeartbeatReceive;

    CHEngineP.BeaconSend -> CHMessagePumpP.AMSend;
    CHEngineP.BeaconPacket = BeaconPacket;
    CHEngineP.BeaconAMPacket = BeaconAMPacket;
    CHEngineP.BeaconReceive = BeaconReceive;

    CHEngineP.CHState -> CHLocalStateP;
    CHEngineP.CHStateInit -> CHLocalStateP;
    CHEngineP.CHClustering -> CHClusteringP;
    CHEngineP.CHRouting -> CHRoutingP;
    CHEngineP.CHStateDebug -> CHLocalStateP;

    CHEngineP.MsgPool = MessagePool;
    CHEngineP.MsgPumpControl -> CHMessagePumpP.Control;

    CHEngineP.Random = Random;


    CHClusteringP.Random = Random;


    CHRoutingP.CHState -> CHLocalStateP;
    CHRoutingP.CHClustering -> CHClusteringP;
    CHRoutingP.CHConfig -> CHConfigP;
    CHRoutingP.NeighborTable = NeighborTable;
    CHRoutingP.MessageLabel -> CHLabelP;
    CHRoutingP.Random = Random;


    CHMessagePumpP.SubAMSend = BeaconSend;
    CHMessagePumpP.PumpTimer -> CHMessagePumpTimerP;
    CHMessagePumpP.Random = Random;

}
