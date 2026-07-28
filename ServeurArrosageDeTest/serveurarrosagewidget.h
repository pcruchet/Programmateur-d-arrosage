#ifndef SERVEURARROSAGEWIDGET_H
#define SERVEURARROSAGEWIDGET_H

#include <QWidget>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>

QT_BEGIN_NAMESPACE
namespace Ui {
class ServeurArrosageWidget;
}
QT_END_NAMESPACE

class ServeurArrosageWidget : public QWidget
{
    Q_OBJECT

public:
    ServeurArrosageWidget(QWidget *parent = nullptr);
    ~ServeurArrosageWidget();

private slots:
    void on_Lancer_clicked();
    void on_pushButtonEnvoyer_clicked();
    void onMessageReceived(const QString &_msg);

    void on_pushButtonEffacer_clicked();

private:
    Ui::ServeurArrosageWidget *ui;
    QWebSocketServer *serveur = nullptr;
    QList<QWebSocket*> clients;

    bool serveurActif = false;
};
#endif // SERVEURARROSAGEWIDGET_H
