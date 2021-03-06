#ifndef CVIRTUALMACHINE_H
#define CVIRTUALMACHINE_H

#include <stdint.h>
#include "IVirtualMachine.h"

class CVirtualMachineLinux : public IVirtualMachine
{
private:

  CVirtualMachineLinux(const CVirtualMachineLinux &vm);
  CVirtualMachineLinux& operator=(const CVirtualMachineLinux& vm);

public:
  explicit CVirtualMachineLinux(IMachine* xpcom_machine, ISession *session);
  virtual ~CVirtualMachineLinux(void);

  virtual nsresult launch_vm(vb_launch_mode_t mode,
                     IProgress** progress);

  virtual nsresult save_state(IProgress** progress);
  virtual nsresult turn_off(IProgress** progress);
  virtual nsresult pause();
  virtual nsresult resume();
  virtual nsresult remove(IProgress** progress) ;
  virtual nsresult run_process(const char* path,
                               const char* user,
                               const char* password,
                               int argc,
                               const char** argv);
};


#endif // CVIRTUALMACHINE_H
