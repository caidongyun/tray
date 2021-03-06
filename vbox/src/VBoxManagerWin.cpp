#include <QApplication>
#include <stdint.h>
#include <VBox/com/com.h>
#include <VBox/com/ptr.h>
#include <VBox/com/array.h>

#include <QDebug>
#include "VBoxManagerWin.h"
#include "VirtualMachineWin.h"
#include "NotifiactionObserver.h"
#include <QMessageBox>

typedef BOOL WINBOOL;

CVBoxManagerWin::CVBoxManagerWin() :
  m_virtual_box(NULL),
  m_event_source(NULL),
  m_event_listener(NULL)
{
#define CHECK_RES(x) if (FAILED(m_last_error)) { \
  qDebug() << QString("err : %1").arg(m_last_error, 1, 16); \
  m_last_vb_error = x; \
  break; \
}

  do {
    m_last_error = com::Initialize(true);
    CHECK_RES(VBE_INIT_XPCOM2);

    CComModule _Module;
    CAtlModule* _pAtlModule = &_Module;
    UNUSED_ARG(_pAtlModule);

    m_last_error = CoCreateInstance(CLSID_VirtualBox,
                                    NULL,
                                    CLSCTX_LOCAL_SERVER,
                                    IID_IVirtualBox,
                                    (void**)&m_virtual_box);
    CHECK_RES(VBE_CREATE_VIRTUAL_BOX);

    m_last_error = m_virtual_box->get_EventSource(&m_event_source);
    CHECK_RES(VBE_EVENT_SOURCE);    

//    m_last_error = m_event_source->CreateListener(&m_event_listener);
//    CHECK_RES(VBE_EVENT_LISTENER);

    ComObjPtr<CEventListenerWinImpl> pListener;
    pListener.createObject();
    pListener->init(new CEventListenerWin, this);

    /*why do we need this? comment for now*/
//    SAFEARRAYBOUND bound;
//    bound.lLbound = 0;
//    bound.cElements = (ULONG) m_dct_event_handlers.size();

    com::SafeArray<VBoxEventType_T> safe_interested;
    for (auto i = m_dct_event_handlers.cbegin(); i != m_dct_event_handlers.cend(); ++i) {
      safe_interested.push_back((VBoxEventType_T) i->first);
    }

    m_last_error = m_event_source->RegisterListener(pListener,
                                                    ComSafeArrayAsInParam(safe_interested),
                                                    TRUE);
    CHECK_RES(VBE_REGISTER_LISTENER);
  } while (false);
#undef CHECK_RES
}

CVBoxManagerWin::~CVBoxManagerWin() {
  if (m_event_source) {
    m_event_source->UnregisterListener(m_event_listener);
    m_event_source->Release();
  }
//  m_event_listener->Release();

  if (m_virtual_box) m_virtual_box->Release();

  m_event_source = NULL;
  m_event_listener = NULL;
  m_virtual_box = NULL;

  com::Shutdown();
}
////////////////////////////////////////////////////////////////////////////

com::Bstr CVBoxManagerWin::machine_id_from_machine_event(IEvent *event) {
  IMachineEvent* me_event;
  com::Bstr res;
  nsresult rc = event->QueryInterface(IID_IMachineEvent, (void**)&me_event);
  if (FAILED(rc)) return com::Bstr("");
  rc = me_event->get_MachineId(res.asOutParam());
  if (FAILED(rc)) return com::Bstr("");
  return res;
}
////////////////////////////////////////////////////////////////////////////

void CVBoxManagerWin::on_machine_state_changed(IEvent *event) {
  IMachineStateChangedEvent* msc_event;
  event->QueryInterface(IID_IMachineStateChangedEvent, (void**)&msc_event);
  com::Bstr str_machine_id = machine_id_from_machine_event(event);
  MachineState new_state;
  msc_event->get_State(&new_state);
  if (m_dct_machines.find(str_machine_id) != m_dct_machines.end())
    m_dct_machines[str_machine_id]->set_state((uint32_t)new_state); //because we can remove VM earlier
  emit vm_state_changed(str_machine_id);
}
////////////////////////////////////////////////////////////////////////////

void CVBoxManagerWin::on_machine_registered(IEvent *event) {
  IMachineRegisteredEvent* mr_event;
  event->QueryInterface(IID_IMachineRegisteredEvent, (void**)&mr_event);
  WINBOOL registered;
  mr_event->get_Registered(&registered);
  com::Bstr str_machine_id = machine_id_from_machine_event(event);

  if(registered != TRUE) {
    if (m_dct_machines[str_machine_id])
      delete m_dct_machines[str_machine_id];
    m_dct_machines.erase(m_dct_machines.find(str_machine_id));
    emit vm_remove(str_machine_id);
    return;
  }

  IMachine *machine;
  nsresult rc;

  rc = m_virtual_box->FindMachine(str_machine_id.raw(), &machine);
  if (FAILED(rc)) return;

  BOOL accessible = FALSE;
  machine->get_Accessible(&accessible);
  if (accessible == FALSE) {
      qDebug() << "not accessible\n";
      return;
  }

  BSTR vm_name;
  machine->get_Name(&vm_name);

  QString name = QString::fromUtf16((ushort*)vm_name);
  qDebug() << "machine name " << name << "\n";
  if (!name.contains("subutai"))
      return;

  IVirtualMachine *vm = new CVirtualMachineWin(machine);
  m_dct_machines[vm->id()] = vm;
  emit vm_add(vm->id());
}
////////////////////////////////////////////////////////////////////////////

void CVBoxManagerWin::on_session_state_changed(IEvent *event) {
  ISessionStateChangedEvent* ssc_event;
  event->QueryInterface(IID_ISessionStateChangedEvent, (void**)&ssc_event);
  SessionState state;
  ssc_event->get_State(&state);  
  com::Bstr str_machine_id = machine_id_from_machine_event(event);
  emit vm_session_state_changed(str_machine_id);
}
////////////////////////////////////////////////////////////////////////////

void CVBoxManagerWin::on_machine_event(IEvent *event) {
  UNUSED_ARG(event);
  qDebug() << "on_machine_event";
}
////////////////////////////////////////////////////////////////////////////

int CVBoxManagerWin::init_machines() {
  IMachine **machines;
  nsresult rc;
  SAFEARRAY* safe_machines = NULL;
  uint32_t count, rh_count = 0;

  if (m_last_vb_error != VBE_SUCCESS)
    return m_last_vb_error;

  rc = m_virtual_box->get_Machines(&safe_machines);
  if (FAILED(rc)) return VBE_GET_MACHINES;

  count = safe_machines->rgsabound[0].cElements;
  if (count == 0)
    return -1;

  rc = SafeArrayAccessData(safe_machines, (void**) &machines);
  if (FAILED(rc)) return VBE_SAFE_ARRAY_ACCESS_DATA;

  do {
    --count;
    if (!machines[count]) continue;
    WINBOOL accesible = FALSE;
    machines[count]->get_Accessible(&accesible);
    if (accesible == FALSE) continue;

    BSTR vm_name;
    machines[count]->get_Name(&vm_name);

    QString name = QString::fromUtf16((ushort*)vm_name);
    qDebug() << "machine name " << name << "\n";
    if (!name.contains("subutai"))
        continue;
    rh_count++;
    IVirtualMachine* vm = new CVirtualMachineWin(machines[count]);
    m_dct_machines[vm->id()] = vm;
  } while (count != 0);

  SafeArrayUnaccessData(safe_machines);
  //SafeArrayDestroy(safe_machines);
  return (rh_count == 0) ? 1 : 0;
}
////////////////////////////////////////////////////////////////////////////

#define HANDLE_PROGRESS(sig, prog) do { \
    ULONG percent = 0; \
    WINBOOL completed = FALSE; \
    for (int i = 0; i < 10 && completed == FALSE; ++i) { \
      rc = prog->WaitForCompletion(10); \
      prog->get_Percent(&percent); \
      prog->get_Completed(&completed); \
      QApplication::processEvents(); \
      emit sig(vm_id, (uint32_t)percent); \
    } \
  } while(0);

int CVBoxManagerWin::launch_vm(const com::Bstr &vm_id,
                               vb_launch_mode_t lm) {
  if (m_dct_machines.find(vm_id) == m_dct_machines.end())
    return 1;
  nsresult rc, state;
  state = m_dct_machines[vm_id]->state();
  if (state == VMS_Aborted) {
      //return 1;//aborted machine will run
  }

  if (state == VMS_Stuck) {
      return 2;
  }

  if( (int)state >= 8 && (int)state <= 23 ) //transition
  {
      qDebug() << "transition state\n" ;
      return 4;
  }

  if( (int)state >= 5 && (int)state <= 19 )
    {
      qDebug() << "turned on already \n" ;
      return 3;//1;
  }

  IProgress* progress = NULL;
  rc = m_dct_machines[vm_id]->launch_vm(lm, &progress);

  if (FAILED(rc)) {   
     return 9;
  }
  HANDLE_PROGRESS(vm_launch_progress, progress);
  return 0;
}
////////////////////////////////////////////////////////////////////////////

int CVBoxManagerWin::turn_off(const com::Bstr &vm_id,
                              bool save_state) {
  if (m_dct_machines.find(vm_id) == m_dct_machines.end())
    return 1;

  nsresult rc, state;
  state = m_dct_machines[vm_id]->state();
  if (state == VMS_Aborted) {
      return 11;//Impossible
  }

  if (state == VMS_Stuck) {
      return 2;
  }

  if( (int)state >= 8 && (int)state <= 23 ) //transition
  {
      qDebug() << "transition state\n" ;
      return 4;
  }

  if( (int)state < 5 )
    {
      qDebug() << "turned on already \n" ;
      return 5;//1;
  }

  if (save_state) {
      IProgress* progress;
      rc = m_dct_machines[vm_id]->save_state(&progress);
      if (FAILED(rc)){
          return 10;
      }
     HANDLE_PROGRESS(vm_save_state_progress, progress);
  }

  IProgress* progress;
  rc = m_dct_machines[vm_id]->turn_off(&progress);
  if (FAILED(rc)){
      return 19;
  }

  HANDLE_PROGRESS(vm_turn_off_progress, progress);
  return 0;
}
////////////////////////////////////////////////////////////////////////////

int CVBoxManagerWin::pause(const com::Bstr &vm_id) {
// Machine can be Paused only from Running/Teleporting/LiveSnapShotting State = 5

  if (m_dct_machines.find(vm_id) == m_dct_machines.end())
    return 1;
  nsresult rc, state;
  state = m_dct_machines[vm_id]->state();
  if( (int)state != 5 && (int)state != 8 && (int)state != 9 )
  {
    qDebug() << "not in running state \n" ;
    return 6;//1;
  }

  rc = m_dct_machines[vm_id]->pause();
  if (FAILED(rc)) {
    return 19;
  }

  //HANDLE_PROGRESS(vm_turn_off_progress, progress);
  return rc;
}
////////////////////////////////////////////////////////////////////////////

int CVBoxManagerWin::resume(const com::Bstr &vm_id) {
// Machine can be Paused only from Running/Teleporting/LiveSnapShotting State = 5

  if (m_dct_machines.find(vm_id) == m_dct_machines.end())
    return 1;
  nsresult rc, state;
  state = m_dct_machines[vm_id]->state();
  if(  (int)state != 6  )
  {
    qDebug() << "not in paused state \n" ;
    return 6;//1;
  }

  rc = m_dct_machines[vm_id]->resume();
  if (FAILED(rc)) {
    return 21;
  }

  //HANDLE_PROGRESS(vm_turn_off_progress, progress);
  return rc;
}

////////////////////////////////////////////////////////////////////////////

int CVBoxManagerWin::remove(const com::Bstr &vm_id) {
// Machine can be Removed if State < 5

  if (m_dct_machines.find(vm_id) == m_dct_machines.end())
    return 1;
  nsresult rc, state;

  QMessageBox msg;
  msg.setIcon(QMessageBox::Question);
  msg.setText("Virtual machine will be removed! ");
  msg.setInformativeText("Are You sure?");
  msg.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
  msg.setDefaultButton(QMessageBox::Cancel);
  int mret = msg.exec();
  switch (mret) {
  case QMessageBox::Cancel:
      qDebug() << "cancel pressed \n";
      return 23;
      break;
  case QMessageBox::Ok:
      qDebug() << "ok pressed \n";
      break;
  default:
      return 23;
      break;
  }
  state = m_dct_machines[vm_id]->state();
  qDebug() << " state: " << (int)state << "\n" ;
  if(  (int)state > 4  )
  {
    qDebug() << "not in poweroff state \n" ;

//  CNotifiactionObserver::NotifyAboutError(QString("Remove machine failed. Power off machine first"));
    msg.setIcon(QMessageBox::Information);
    msg.setText(tr("Virtual machine can not be removed!"));
    msg.setInformativeText(tr("Power off machine first"));
    msg.setStandardButtons(QMessageBox::Ok);
    mret = msg.exec();
    return 6;//1;
  }

  IProgress* progress;
  qDebug() << "VBoxManagerWin before remove vm " << "\n";
  rc = m_dct_machines[vm_id]->remove(&progress);
  qDebug() << "VBoxManagerWin after remove \n";
  if (FAILED(rc)) {
    return 23;
  }

  //HANDLE_PROGRESS(vm_turn_off_progress, progress);
  return rc;
}


////////////////////////////////////////////////////////////////////////////

int CVBoxManagerWin::add(const com::Bstr &vm_id) {
// Machine can be Removed if State < 5

//dialog?
  nsresult rc;
  IMachine *newmachine = NULL;
  BSTR vm_idd = SysAllocString(L"test-add");
  if (m_dct_machines.find(vm_id)!= m_dct_machines.end()){
    qDebug() << "vm exists \n" ;
    //return 23;//vm exists
  }
  rc = m_virtual_box->CreateMachine(NULL,
                                    vm_idd,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &newmachine);
  if (FAILED(rc)) {
      qDebug() << "create failed \n" ;
    return 23;
  }
  rc = newmachine->put_MemorySize(128);
  if (FAILED(rc)) {
      qDebug() << "memsize failed \n" ;
    return 23;
  }

//          virtual HRESULT STDMETHODCALLTYPE CreateMachine(
//              /* [in] */ BSTR aSettingsFile,
//              /* [in] */ BSTR aName,
//              /* [in] */ SAFEARRAY * aGroups,
//              /* [in] */ BSTR aOsTypeId,
//              /* [in] */ BSTR aFlags,
//              /* [retval][out] */ IMachine **aMachine) = 0;

  //HANDLE_PROGRESS(vm_turn_off_progress, progress);
  rc = m_virtual_box->RegisterMachine(newmachine);
  if (FAILED(rc)) {
      qDebug() << "register failed \n" ;
    return 23;
  }

  return rc;
}
////////////////////////////////////////////////////////////////////////////

QString CVBoxManagerWin::version() {
  com::Bstr ver("");
  nsresult rc = m_virtual_box->get_Version(ver.asOutParam());
  if (SUCCEEDED(rc)) {
    QString result((QChar*)ver.raw(), (int)ver.length());
    return result;
  }
  return "";
}

////////////////////////////////////////////////////////////////////////////

//int CVBoxManagerWin::find_net_address(const com::Bstr &vm_id) {
//  if (m_dct_machines.find(vm_id) == m_dct_machines.end())
//      return 1;
//  nsresult rc, state;
//  IMachine *m =  m_dct_machines[vm_id];
//  state = m_dct_machines[vm_id]->state();
//  if( (int)state != 5 )
//  {
//    qDebug() << "not in running state \n" ;
//    return 6;//1;
//  }
//  BSTR pvalue;
//  BSTR pproperty = SysAllocString(L"test-add");
//  QString name = QString::fromUtf16((ushort*)vm_name);
//  m->GetGuestPropertyValue();
//}

////////////////////////////////////////////////////////////////////////////

HRESULT CEventListenerWin::HandleEvent(VBoxEventType e_type, IEvent *pEvent) {
  return m_vbox_manager->handle_event(e_type, pEvent);
}
