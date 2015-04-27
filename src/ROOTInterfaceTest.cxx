#include "TSystem.h"
#include "RQ_OBJECT.h"
#include "Rtypes.h"




#include <iostream>
#include <ostream>
#include <vector>
#include <memory>
#include <string>
#include <fstream>

#ifndef __CINT__
#include "ROOTProducer.h"
#endif

const char* defaultFileName= "c:/slac/EventData_288_75.dat";

class  TSignals {
  RQ_OBJECT("TSignals")
public:
  TSignals() {}
  void StartOfBurst() { Emit("StartOfBurst()"); }
  void StartEvent(int nev) { Emit("StartEvent(int)", nev); }
  void EventData(int id, int nstrips, bool* data0, bool* data1) {
    EmitVA("EventData(int,int,bool*,bool*)", 4, id, nstrips, data0, data1);
  }
  void EndEvent(int nev) { Emit("EndEvent(int)", nev); }
  void EndOfBurst() { Emit("EndOfBurst()"); }
  Bool_t ConnectCINT(const char *signal,
    const char *slot) {
    return ::ConnectCINT(&fQObject, signal, slot);
  }
  ClassDef(TSignals, 1);
};



class TSignals;
class ROOTProducer;
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// SCT DUMMY /////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class SCTDummy{
public:
  SCTDummy();
  ~SCTDummy();
  void openfile(const char* name);
  void openfile(std::string& name);

  void setReadoutSpeed(int speed);
  int getTimestamp();

  bool  readLine();

  bool hasData();
  void readOutLoop();
  static const size_t size = 256;
  bool data[size];
  bool data1[size];
  UChar_t data_char[size];
  ULong64_t* data_ULong64;
  TSignals *eSignals;

  int _readoutSpeed = 1;
  ROOTProducer *p;
  ROOTProducer *p1;
  size_t m_ev; //0
  size_t  m_max; //100000
private:
  std::ifstream in;
 
  std::vector < std::string > m_data_vec;
  
};




//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// runSCTClass  //////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class runSCTClass{
  RQ_OBJECT("runSCTClass")
public:
  runSCTClass(const char* name);


  void onStart(int);
  void onConfigure();
  void onStop();
  void OnTerminate();



  void readoutloop();






  enum status_enum{
    unconfigured,
    configured,
    starting,
    started,
    stopping,
    doTerminat

  } m_status;
  ROOTProducer *p;
  ROOTProducer* p1;
  SCTDummy *e;
  ClassDef(runSCTClass, 1);
};




runSCTClass::runSCTClass(const char* name)
{
  p = new ROOTProducer();
  p1 = new ROOTProducer();
  do
  {
    gSystem->Sleep(200);
    p->Connect2RunControl(name, "127.0.0.1:44000");

  } while (!p->getConnectionStatus());
     gSystem->Sleep(2000);
  do{
  gSystem->Sleep(2000);
  p1->Connect2RunControl("test", "127.0.0.1:44000");
  
  }while ( !p1->getConnectionStatus());
    gSystem->Sleep(2000);
  e = new SCTDummy();
  e->p = p;
  e->p1 = p1;
  m_status = status_enum::unconfigured;
  p->addDataPointer_bool(0, e->data, e->size);
  p->addDataPointer_bool(1, e->data1, e->size);
  p->addDataPointer_UChar_t(2, e->data_char, e->size);
  p->addDataPointer_ULong64_t(3, e->data_ULong64, e->size);

  p->Connect("send_onStart()", this->Class_Name(), this, "onStart()");
  p->setTimeOut(10000);
  p->Connect("send_onConfigure()", this->Class_Name(), this, "onConfigure()");
  p->Connect("send_onStop()", this->Class_Name(), this, "onStop()");
  p->Connect("send_OnTerminate()", this->Class_Name(), this, "OnTerminate()");
  e->eSignals->Connect("StartEvent(int)", p->Class_Name(), p, "createNewEvent(int)");
  e->eSignals->Connect("EndEvent(int)", p->Class_Name(), p, "sendEvent(int)");
  
  


 p1->addDataPointer_bool(0, e->data, e->size);
 p1->addDataPointer_UChar_t(1, e->data_char, e->size);
 p1->addDataPointer_ULong64_t(2, e->data_ULong64, e->size);

 p1->Connect("send_onStart()", this->Class_Name(), this, "onStart()");
 p1->Connect("send_onConfigure()", this->Class_Name(), this, "onConfigure()");
 p1->Connect("send_onStop()", this->Class_Name(), this, "onStop()");
 p1->Connect("send_OnTerminate()", this->Class_Name(), this, "OnTerminate()");
  
 p1->setTimeOut(10000); 
 e->eSignals->Connect("StartEvent(int)",  p1->Class_Name(), p1, "createNewEvent(int)");
 e->eSignals->Connect("EndEvent(int)",  p1->Class_Name(), p1, "sendEvent(int)");
  
  
   p1->Connect("send_statusChanged()", p->Class_Name(), p, "checkStatus()");
   p->Connect("send_statusChanged()", p1->Class_Name(), p1, "checkStatus()");
  
  
}

void runSCTClass::onStart(int Run)
{
  std::cout << "<runSCTClass::onStart() "<<Run << std::endl;
  e->m_ev=0;
  e->m_max = 1000000;
  m_status = started;
}

void runSCTClass::onConfigure()
{
  
  std::cout << "runSCTClass::onConfigure()" << std::endl;
  const int sizeofDummy = 256;
  char dummy[sizeofDummy];
  p->getConfiguration("infile", defaultFileName , &dummy[0], sizeofDummy);
  
  e->_readoutSpeed = p1->getConfiguration("readout", 1);
  std::cout << "readout: " << e->_readoutSpeed << std::endl;
  e->openfile(dummy);
  m_status = configured;
  
  
  const int sizeofDummy1 = 256;
  char dummy1[sizeofDummy1];
  p1->getConfiguration("bla", "testparam", &dummy1[0], sizeofDummy1);
 std::cout<<dummy1<<std::endl;
}

void runSCTClass::onStop()
{
  std::cout << "runSCTClass::onStop()" << std::endl;
  e->m_ev =e->m_max+1;
  m_status = stopping;
}

void runSCTClass::OnTerminate()
{
  std::cout << "runSCTClass::OnTerminate()" << std::endl;
  m_status = doTerminat;
}

void runSCTClass::readoutloop()
{
std::cout<< "ready to go" <<Std::endl;
  while (m_status != doTerminat)
  {
     p->checkStatus();
    
     if (m_status != started)
    {
     
      gSystem->Sleep(20);
     
       continue;
    }
    gSystem->Sleep(1000);
    e->readOutLoop();
  }
}







bool SCTDummy::hasData()
{
  gSystem->Sleep(_readoutSpeed);

  return m_ev<m_max;
}

bool SCTDummy::readLine()
{
  size_t ev = m_ev * 2 + 1;
  std::string line = m_data_vec.at(ev%m_data_vec.size());

  size_t j = 0;
  for (int i = 0; i < line.size(); ++i)
  {

    if (j>=size)
    {
      continue;
    }
    if (line.at(i) == '1')
    {
      data[j] = true;
      data_char[j]=j; // to have not always the same number 
      data_ULong64[j]=j*1000;
      data1[size-j]=true;
    }
    else
    {
      data[j] = false;
      data_char[j] =0;
      data_ULong64[j]=0;
      data1[size-j]=false;
    }
    ++j;
  }





  return true;
}

void SCTDummy::setReadoutSpeed(int speed)
{
  _readoutSpeed = speed;
}

void SCTDummy::openfile(const char* name)
{

  std::cout << "opening file: " << name << std::endl;
  in.open(name);

  std::string line;
  while (!in.eof())
  {
    std::getline(in, line);
    if (line.empty())
    {
      continue;
    }
    m_data_vec.push_back(line);
  }
  in.close();
  std::cout << "size of file: " << m_data_vec.size() << std::endl;

}

void SCTDummy::openfile(std::string& name)
{
  openfile(name.c_str());
}

SCTDummy::~SCTDummy()
{
  delete eSignals;
delete data_ULong64;
  if (in)
  {
    in.close();
  }
}

int SCTDummy::getTimestamp()
{
  return 0;
}

void SCTDummy::readOutLoop()
{
  eSignals->StartOfBurst();
  while (hasData())
  {
    eSignals->StartEvent(m_ev);

    
    
    
    readLine();

    
   
    eSignals->EndEvent(m_ev);
    ++m_ev;
   //std::cout<< "send event "<< m_ev <<std::endl;
  }
  eSignals->EndOfBurst();
}

SCTDummy::SCTDummy()
{
  m_ev = 0;
  m_max = 1000000;
  eSignals = new TSignals();
  data_ULong64 =new ULong64_t[size];
  for (int i = 0; i < size; ++i)
  {
    data[i] = 0;
    data_char[i]=0;
    data_ULong64[i]=0;
  }
}


void ROOTInterfaceTest(const char* name){

  gSystem->Load("ROOTProducer.dll");

  runSCTClass rSCT(name);

  rSCT.readoutloop();

gSystem->Exit(0);
}


void ROOTInterfaceTest(){

  gSystem->Load("ROOTProducer.dll");

  runSCTClass rSCT("sct1");

  rSCT.readoutloop();

gSystem->Exit(0);
}

#ifndef __CINT__
int main(){
  ROOTInterfaceTest();
  std::cout << "end" << std::endl;
}

#endif