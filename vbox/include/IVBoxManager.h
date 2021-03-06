#ifndef VBOXMANAGER_H
#define VBOXMANAGER_H


#include <stdint.h>
#include <VBox/com/string.h>
#include <map>
#include <QObject>

#include "VirtualBoxHdr.h"
#include "VBoxCommons.h"
#include "IVirtualMachine.h"

class CVBoxManagerSingleton;

class IVBoxManager : public QObject {
  Q_OBJECT

private:
  friend void* passive_event_listener(void* manager);
  friend class CVBoxManagerSingleton;

protected:
  IVBoxManager();
  virtual ~IVBoxManager();

  nsresult m_last_error;
  vb_errors_t m_last_vb_error;

  std::map<com::Bstr, IVirtualMachine*> m_dct_machines;
  typedef void (IVBoxManager::*pf_event_handle)(IEvent*);
  std::map<uint32_t, pf_event_handle> m_dct_event_handlers;

  virtual void on_machine_state_changed(IEvent* event) = 0;
  virtual void on_machine_registered(IEvent* event) = 0;
  virtual void on_session_state_changed(IEvent* event) = 0;
  virtual void on_machine_event(IEvent* event) = 0;

  void init_event_handlers(void);

public:
  nsresult last_error(void) const {return m_last_error;}
  vb_errors_t last_vb_error(void) const {return m_last_vb_error;}

  const IVirtualMachine* vm_by_id(const com::Bstr& id) const {
    return m_dct_machines.find(id) == m_dct_machines.end() ? NULL : m_dct_machines.at(id);}

  size_t vm_count() const {return m_dct_machines.size();}
  const std::map<com::Bstr, IVirtualMachine*>& dct_machines() const {return m_dct_machines;}

  virtual int init_machines(void) = 0;

  virtual int launch_vm(const com::Bstr& vm_id,
                        vb_launch_mode_t lm = VBML_HEADLESS) = 0;

  virtual int turn_off(const com::Bstr& vm_id,
                       bool save_state = false) = 0;

  int launch_process(const com::Bstr& vm_id,
                     const char* path,
                     const char* user,
                     const char* password,
                     const char** argv,
                     int argc);

  virtual int pause(const com::Bstr& vm_id) = 0;
  virtual int resume(const com::Bstr& vm_id) = 0;
  virtual int remove(const com::Bstr& vm_id) = 0;
  virtual int add(const com::Bstr& vm_id) = 0;
  virtual QString version() = 0;

  nsresult handle_event(VBoxEventType_T e_type,
                        IEvent *event);
  void init_com();
  void shutdown_com();

signals:
  void vm_add(const com::Bstr& vm_id);
  void vm_remove(const com::Bstr& vm_id);
  void vm_state_changed(const com::Bstr& vm_id);
  void vm_session_state_changed(const com::Bstr& vm_id);
  void vm_launch_progress(const com::Bstr& vm_id, uint32_t percent);
  void vm_save_state_progress(const com::Bstr& vm_id, uint32_t percent);
  void vm_turn_off_progress(const com::Bstr& vm_id, uint32_t percent);
};
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

class CVBoxManagerSingleton {
private:
  class CVBoxManagerDestructor {
  private:
    IVBoxManager* m_instance;
  public:
    CVBoxManagerDestructor() : m_instance(NULL) {}
    ~CVBoxManagerDestructor(void) {if (m_instance) delete m_instance;}
    void Init(IVBoxManager* instance) {m_instance = instance;}
  };

  static IVBoxManager* p_instance;
  static CVBoxManagerDestructor p_destructor;

public:
  static IVBoxManager* Instance();
};
#endif // VBOXMANAGER_H
