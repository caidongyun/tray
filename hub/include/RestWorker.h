#ifndef CRESTWORKER_H
#define CRESTWORKER_H

#include <QObject>
#include <QString>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSsl>
#include <QtNetwork/QSslConfiguration>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>

typedef enum error_login {
  EL_SUCCESS = 0,
  EL_HTTP = 1,
  EL_LOGIN_OR_EMAIL = 2,
  EL_TIMEOUT = 3,
  EL_NOT_JSON_DOC = 4,
  EL_NOT_JSON_OBJECT = 5
} error_login_t;

class CHubContainer {
private:
  std::pair<QString, QString> m_ip_name;

public:
  explicit CHubContainer(QJsonObject obj) :
     m_ip_name(obj["ip"].toString(), obj["name"].toString())
  {
  }

  ~CHubContainer(){}

  const QString& ip() const { return m_ip_name.first; }
  const QString& name() const { return m_ip_name.second; }

  bool operator==(const CHubContainer& arg) const {
    return m_ip_name == arg.m_ip_name;
  }

  bool operator!=(const CHubContainer& arg) const {
    return !(this->operator ==(arg));
  }
};

class CSSEnvironment {
private:
  QString m_name;
  QString m_hash;
  QString m_aes_key;
  QString m_ttl;
  std::vector<CHubContainer> m_lst_containers;
public:
  CSSEnvironment() : m_name(""){}
  explicit CSSEnvironment(const QJsonObject& obj) {
    m_name = obj["name"].toString();
    m_hash = obj["hash"].toString();
    m_aes_key = obj["key"].toString();
    m_ttl = obj["ttl"].toString();

    QJsonArray arr = obj["containers"].toArray();
    for (size_t i = 0; i < arr.size(); ++i) {
      QJsonObject item = arr.at(i).toObject();
      m_lst_containers.push_back(CHubContainer(item));
    }
  }

  ~CSSEnvironment(){}

  bool operator==(const CSSEnvironment& arg) const {
    return m_name == arg.m_name &&
        m_hash == arg.m_hash &&
        m_aes_key == arg.m_aes_key &&
        m_ttl == arg.m_ttl &&
        m_lst_containers == arg.m_lst_containers;
  }

  bool operator!=(const CSSEnvironment& arg) const {
    return !(this->operator ==(arg));
  }

  const QString& name() const {return m_name;}
  const QString& hash() const {return m_hash;}
  const QString& key() const {return m_aes_key;}
  const std::vector<CHubContainer>& containers() const {return m_lst_containers;}
};
////////////////////////////////////////////////////////////////////////////

//class CSSContainer {
//private:
//  std::map<QString, std::vector<QString> > m_dct_env_containers;
//public:
//  CSSContainer(){}
//  explicit CSSContainer(const QJsonObject& json_obj) {
//    for (int i = 0; i < json_obj.keys().count(); ++i) {
//      QJsonArray arr = json_obj[json_obj.keys().at(i)].toArray();
//      for (int j = 0; j < arr.size(); ++j) {
//        m_dct_env_containers[json_obj.keys().at(i)].push_back(arr.at(j).toString());
//      }
//    }
//  }
//  explicit CSSContainer(const std::map<QString, std::vector<QString> >& dct) : m_dct_env_containers(dct) {}
//  ~CSSContainer(void){}
//  const std::map<QString, std::vector<QString> >& dct_env_containers() const {return m_dct_env_containers;}
//  bool operator==(const CSSContainer& arg) {
//    return m_dct_env_containers == arg.m_dct_env_containers;
//  }
//  bool operator!=(const CSSContainer& arg) {
//    return !(this->operator ==(arg));
//  }
//};
////////////////////////////////////////////////////////////////////////////

class CSSPeerUser {
private:
  std::map<QString, std::vector<QString> > m_dct_peer_users;
public:
  CSSPeerUser(){}
  explicit CSSPeerUser(const QJsonObject& json_obj) {
    for (int i = 0; i < json_obj.keys().count(); ++i) {
      QJsonArray arr = json_obj[json_obj.keys().at(i)].toArray();
      for (int j = 0; j < arr.size(); ++j) {
        m_dct_peer_users[json_obj.keys().at(i)].push_back(arr.at(j).toString());
      }
    }
  }
  explicit CSSPeerUser(const std::map<QString, std::vector<QString> >& dct) : m_dct_peer_users(dct) {}
  ~CSSPeerUser() {}
  const std::map<QString, std::vector<QString> >& dct_peer_users() const {return m_dct_peer_users;}
  bool operator==(const CSSPeerUser& arg) {
    return m_dct_peer_users == arg.m_dct_peer_users;
  }
  bool operator!=(const CSSPeerUser& arg) {
    return !(this->operator ==(arg));
  }
};
////////////////////////////////////////////////////////////////////////////

class CSSBalance {
private:
  QString m_balance;
public:
  CSSBalance() : m_balance("") {}
  explicit CSSBalance(const QString& balance) : m_balance(balance) {
  }
  ~CSSBalance(){}

  const QString& value() const {return m_balance;}
};
////////////////////////////////////////////////////////////////////////////

class CRestWorker {
private:

  static QByteArray send_request(const QNetworkRequest& req, bool get, int &http_status_code, int &err_code);
  static QByteArray get_request(const QNetworkRequest& req, int &http_status_code, int &err_code);
  static QByteArray post_request(const QNetworkRequest& req, int &http_status_code, int &err_code);
  static QJsonDocument get_request_json_document(const QString& link, int& http_code, int &err_code);

  CRestWorker();
  CRestWorker(const QString& login,
              const QString& password);
  CRestWorker(const CRestWorker& worker);
  ~CRestWorker(void);
public:

  static int login(const QString& login, const QString& password);
  static std::vector<CSSEnvironment> get_environments(int &http_code, int& err_code);
//  static CSSContainer get_containers(int &http_code, int& err_code);
  static CSSPeerUser get_peer_users(int &http_code, int& err_code);
  static CSSBalance get_balance(int &http_code, int& err_code);
};

#endif // CRESTWORKER_H
