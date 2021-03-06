/*
 * "Copyright (c) 2005 Stanford University. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose, without fee, and without written
 * agreement is hereby granted, provided that the above copyright
 * notice, the following two paragraphs and the author appear in all
 * copies of this software.
 *
 * IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF STANFORD UNIVERSITY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * STANFORD UNIVERSITY SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND STANFORD UNIVERSITY
 * HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
 * ENHANCEMENTS, OR MODIFICATIONS."
 */

/**
 * Implementation of TOSSDR C++ classes. Generally just directly
 * call their C analogues.
 *
 * @author Philip Levis, Markus Becker
 * @date   Apr 10 2009
 */

#include <stdint.h>
#include <tossdr.h>
#include <sim_tossdr.h>
#include <sim_mote.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hashtable.h>

#include <packet.c>

uint16_t TOS_NODE_ID = 1;

Variable::Variable(char* str, char* formatStr, int array, int which) {
  name = str;
  format = formatStr;
  isArray = array;
  mote = which;

  int sLen = strlen(name);
  realName = (char*)malloc(sLen + 1);
  memcpy(realName, name, sLen + 1);
  realName[sLen] = 0;

  for (int i = 0; i < sLen; i++) {
    if (realName[i] == '.') {
      realName[i] = '$';
    }
  }

  //  printf("Creating %s realName: %s format: %s %s\n", name, realName, formatStr, array? "[]":"");

  if (sim_mote_get_variable_info(mote, realName, &ptr, &len) == 0) {
    data = (char*)malloc(len + 1);
    data[len] = 0;
  }
  else {
    printf("Could not find variable %s\n", realName);
    data = NULL;
    ptr = NULL;
  }
  printf("Allocated variable %s\n", realName);
}

Variable::~Variable() {
  printf("Freeing variable %s\n", realName);
  free(data);
  free(realName);
}

/* This is the sdbm algorithm, taken from
   http://www.cs.yorku.ca/~oz/hash.html -pal */
static unsigned int tossim_hash(void* key) {
  char* str = (char*)key;
  unsigned int hashVal = 0;
  int c;

  while ((c = *str++))
    hashVal = c + (hashVal << 6) + (hashVal << 16) - hashVal;

  return hashVal;
}

static int tossim_hash_eq(void* key1, void* key2) {
  return strcmp((char*)key1, (char*)key2) == 0;
}

variable_string_t Variable::getData() {
  if (data != NULL && ptr != NULL) {
    str.ptr = data;
    str.type = format;
    str.len = len;
    str.isArray = isArray;
    //    printf("Getting %s %s %s\n", format, isArray? "[]":"", name);
    memcpy(data, ptr, len);
  } else {
    str.ptr = (char*)"<no such variable>";
    str.type = (char*)"<no such variable>";
    str.len = strlen("<no such variable>");
    str.isArray = 0;
  }
  return str;
}

/*void Variable::setData(variable_string_t newdata) {
  if (data != NULL && ptr != NULL) {
    str.ptr = data;
    str.type = format;
    str.len = len;
    str.isArray = isArray;
    //    printf("Getting %s %s %s\n", format, isArray? "[]":"", name);
    memcpy(data, ptr, len);
  }
  else {
    str.ptr = (char*)"<no such variable>";
    str.type = (char*)"<no such variable>";
    str.len = strlen("<no such variable>");
    str.isArray = 0;
  }
  return str;
  }*/

Mote::Mote(nesc_app_t* n) {
  app = n;
  varTable = create_hashtable(128, tossim_hash, tossim_hash_eq);
}

Mote::~Mote(){}

unsigned long Mote::id() {
  return nodeID;
}

long long int Mote::euid() {
  return sim_mote_euid(nodeID);
}

void Mote::setEuid(long long int val) {
  sim_mote_set_euid(nodeID, val);
}

long long int Mote::bootTime() {
  return sim_mote_start_time(nodeID);
}

void Mote::bootAtTime(long long int time) {
  sim_mote_set_start_time(nodeID, time);
  sim_mote_enqueue_boot_event(nodeID);
}

bool Mote::isOn() {
  return sim_mote_is_on(nodeID);
}

void Mote::turnOff() {
  sim_mote_turn_off(nodeID);
}

void Mote::turnOn() {
  sim_mote_turn_on(nodeID);
}

void Mote::setID(unsigned long val) {
  nodeID = val;
}

Variable* Mote::getVariable(char* name) {
  char* typeStr = (char*)"";
  int isArray;
  Variable* var;

  var = (Variable*)hashtable_search(varTable, name);
  if (var == NULL) {
    // Could hash this for greater efficiency,
    // but that would either require transformation
    // in Tossim class or a more complex typemap.
    if (app != NULL) {
      for (int i = 0; i < app->numVariables; i++) {
	if(strcmp(name, app->variableNames[i]) == 0) {
	  typeStr = app->variableTypes[i];
	  isArray = app->variableArray[i];
	  break;
	}
      }
    }
    //  printf("Getting variable %s of type %s %s\n", name, typeStr, isArray? "[]" : "");
    var = new Variable(name, typeStr, isArray, nodeID);
    hashtable_insert(varTable, name, var);
  }
  return var;
}

Tossdr::Tossdr(nesc_app_t* n) {
  app = n;
  init();
}

Tossdr::~Tossdr() {
  sim_end();
}

void Tossdr::init() {
  sim_init();
  motes = (Mote**)malloc(sizeof(Mote*) * (TOSSIM_MAX_NODES + 1));
  memset(motes, 0, sizeof(Mote*) * TOSSIM_MAX_NODES);
}

long long int Tossdr::time() {
  return sim_time();
}

long long int Tossdr::ticksPerSecond() {
  return sim_ticks_per_sec();
}

char* Tossdr::timeStr() {
  sim_print_now(timeBuf, 256);
  return timeBuf;
}

void Tossdr::setTime(long long int val) {
  sim_set_time(val);
}

Mote* Tossdr::currentNode() {
  return getNode(sim_node());
}

Mote* Tossdr::getNode(unsigned long nodeID) {
  if (nodeID > TOSSIM_MAX_NODES) {
    nodeID = TOSSIM_MAX_NODES;
    // log an error, asked for an invalid node
  }
  else {
    if (motes[nodeID] == NULL) {
      motes[nodeID] = new Mote(app);
      if (nodeID == TOSSIM_MAX_NODES) {
	motes[nodeID]->setID(0xffff);
      }
      else {
	motes[nodeID]->setID(nodeID);
      }
    }
    return motes[nodeID];
  }
}

void Tossdr::setCurrentNode(unsigned long nodeID) {
  sim_set_node(nodeID);
}

void Tossdr::addChannel(char* channel, FILE* file) {
  sim_add_channel(channel, file);
}

bool Tossdr::removeChannel(char* channel, FILE* file) {
  return sim_remove_channel(channel, file);
}

void Tossdr::randomSeed(int seed) {
  return sim_random_seed(seed);
}

bool Tossdr::runNextEvent() {
    //printf("TosSdr: runNextEvent. &%x %x\n", &cbfct, cbfct);
    //cbfct(0, (char*)"1", 5, tx_data); // mab
  return sim_run_next_event();
}

Packet* Tossdr::newPacket() {
  return new Packet();
}


void Tossdr::registerData2SdrCallback(DATA2SDR_CB fct, void* clientdata) {
    //printf("AM: Callback registration 1 &%x %x, &%x %x\n", &fct, fct, &cbfct, cbfct);
  sim_register_data2sdr_callback(fct, clientdata);
}


void Tossdr::registerSendCallback(SEND_CB fct, void* clientdata) {
    //printf("AM: Callback registration 1 &%x %x, &%x %x\n", &fct, fct, &cbfct, cbfct);
  sim_register_send_callback(fct, clientdata);
}

