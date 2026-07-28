#include "serveurarrosagewidget.h"
#include "ui_serveurarrosagewidget.h"

ServeurArrosageWidget::ServeurArrosageWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ServeurArrosageWidget)
{
    ui->setupUi(this);
}

ServeurArrosageWidget::~ServeurArrosageWidget()
{
    delete ui;
}

void ServeurArrosageWidget::on_Lancer_clicked()
{
    if(!serveurActif)
    {
        int port = ui->spinBoxPort->value();

        serveur = new QWebSocketServer("ServeurArrosageTest",
                                       QWebSocketServer::NonSecureMode,
                                       this);
        connect(serveur,
                &QWebSocketServer::newConnection,
                this,
                []()
                {
                    qDebug() << "NEW CONNECTION EVENT TRIGGERED";
                });

        if(serveur->listen(QHostAddress::Any, port))
        {
            ui->textEditStatut->append("Serveur démarré sur port " + QString::number(port));
            serveurActif = true;

            connect(serveur,
                    &QWebSocketServer::newConnection,
                    this,
                    [this]()
                    {
                        QWebSocket *client = serveur->nextPendingConnection();
                        clients.append(client);

                        ui->textEditStatut->append("Client connecté");

                        connect(client,
                                &QWebSocket::textMessageReceived,
                                this,
                                &ServeurArrosageWidget::onMessageReceived);

                        connect(client,
                                &QWebSocket::disconnected,
                                this,
                                [this, client]()
                                {
                                    clients.removeAll(client);
                                    client->deleteLater();

                                    ui->textEditStatut->append("Client déconnecté");
                                });
                    });
        }
        else
        {
            ui->textEditStatut->append("Erreur démarrage serveur");
            ui->textEditStatut->append(serveur->errorString());
        }
    }
}


void ServeurArrosageWidget::on_pushButtonEnvoyer_clicked()
{
    QString msg = ui->lineEditEnvoie->text();

    for(auto c : clients)
        c->sendTextMessage(msg);

    ui->textEditStatut->append("TX manuel : " + msg);
}

void ServeurArrosageWidget::onMessageReceived(const QString &_msg)
{
    ui->textEditTrames->append(_msg);

    // Simulation simple ESP32
    QJsonDocument doc = QJsonDocument::fromJson(_msg.toUtf8());

    if(!doc.isObject())
        return;

    QJsonObject obj = doc.object();

    QString type = obj["t"].toString();
    QString cmd  = obj["c"].toString();

    QJsonObject response;
    response["v"] = 1;
    response["t"] = "resp";
    response["c"] = cmd;

    // SIMULATION basique
    if(cmd == "get_time")
    {
        response["d"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    }
    else if(cmd == "open")
    {
        response["ok"] = 1;
    }
    else if(cmd == "get_state")
    {
        response["e"] = 1;
        response["m"] = "prog";
    }

    QJsonDocument out(response);

    for(auto c : clients)
        c->sendTextMessage(out.toJson(QJsonDocument::Compact));
}


void ServeurArrosageWidget::on_pushButtonEffacer_clicked()
{
    ui->textEditTrames->clear();
}

