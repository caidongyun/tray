#include <QMessageBox>
#include <QApplication>
#include <QDir>
#include <QFile>

#include "DlgGenerateSshKey.h"
#include "ui_DlgGenerateSshKey.h"
#include "SystemCallWrapper.h"
#include "HubController.h"
#include "SettingsManager.h"
#include "NotifiactionObserver.h"

DlgGenerateSshKey::DlgGenerateSshKey(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DlgGenerateSshKey)
{
  ui->setupUi(this);
  set_key_text();

  connect(ui->btn_generate, SIGNAL(released()), this, SLOT(btn_generate_released()));
}

DlgGenerateSshKey::~DlgGenerateSshKey()
{
  delete ui;
}
////////////////////////////////////////////////////////////////////////////

void
DlgGenerateSshKey::generate_new_ssh() {
  QString path = CSettingsManager::Instance().ssh_keys_storage() +
                 QDir::separator() +
                 CHubController::Instance().current_user();
  QFile key(path);
  QFile key_pub(path+".pub");

  if (key.exists() && key_pub.exists()) {
    key.remove();
    key_pub.remove();
  }

  system_call_wrapper_error_t scwe =
      CSystemCallWrapper::generate_ssh_key(CHubController::Instance().current_user().toStdString().c_str(),
                                           path.toStdString().c_str());
  if (scwe != SCWE_SUCCESS) {
    CNotifiactionObserver::Instance()->NotifyAboutError(
          QString("Can't generate ssh-key. Err : %1").arg(CSystemCallWrapper::scwe_error_to_str(scwe)));
    return;
  }
  set_key_text();
}
////////////////////////////////////////////////////////////////////////////

void
DlgGenerateSshKey::set_key_text() {
  QString path = CSettingsManager::Instance().ssh_keys_storage() +
                 QDir::separator() +
                 CHubController::Instance().current_user() + ".pub";
  QFile file(path);
  if (file.exists()) {
    file.open(QFile::ReadOnly);
    QByteArray bytes = file.readAll();
    ui->te_ssh_key->setText(QString(bytes));
  }
}
////////////////////////////////////////////////////////////////////////////

void
DlgGenerateSshKey::btn_generate_released() {
  QFileInfo fi(CSettingsManager::Instance().ssh_keys_storage());
  if (!fi.isDir() || !fi.isWritable()) {
    CNotifiactionObserver::Instance()->NotifyAboutInfo(
          "You don't have write permission to ssh-keys directory. Please change it in settings. Thanks");
    return;
  }

  if (ui->te_ssh_key->toPlainText().isEmpty()) {
    generate_new_ssh();
  } else {
    QMessageBox *msg_question = new QMessageBox;
    do  {
      msg_question->setWindowTitle("New SSH key generation");
      msg_question->setText("Are you sure you want to generate new SSH key? You will need to update your environments");
      msg_question->addButton(QMessageBox::Yes);
      msg_question->addButton(QMessageBox::No);
      msg_question->setDefaultButton(QMessageBox::No);
      if (msg_question->exec() == QMessageBox::No)
        break;
      generate_new_ssh();
    } while (0);
    msg_question->deleteLater();
  }
}
////////////////////////////////////////////////////////////////////////////
