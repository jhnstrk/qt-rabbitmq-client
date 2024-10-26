#include "qtrabbitmq/consumer.h"
#include <qcoreapplication.h>
#include <qtrabbitmq/channel.h>
#include <qtrabbitmq/client.h>
#include <qtrabbitmq/message.h>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QUrl>

int receiverMain(QCoreApplication &app)
{
    qmq::Client client;

    client.connectToHost(QUrl("amqp://localhost"));

    qmq::Consumer consumer;
    QObject::connect(&consumer, &qmq::Consumer::messageReady, [&consumer]() {
        auto message = consumer.dequeueMessage();
        qInfo() << " [x] Received" << message.payload();
    });

    QObject::connect(&client, &qmq::Client::connected, [&]() {
        auto channel = client.createChannel();
        channel->channelOpen().then([&consumer, channel]() {
            const QString queue = "hello";
            const QString msg = "Hello world";
            channel->queueDeclare(queue).then([&consumer, channel, queue](const QVariantList &ret) {
                consumer.consume(channel.get(), queue).then([](const QString &s) {
                    qDebug() << "Consuming" << s;
                });
            });
        });
    });
    qInfo() << " [*] Waiting for messages. To exit press CTRL+C";
    return app.exec();
}

int senderMain(QCoreApplication &app)
{
    qmq::Client client;

    client.connectToHost(QUrl("amqp://localhost"));

    QObject::connect(&client, &qmq::Client::connected, [&]() {
        auto channel = client.createChannel();
        channel->channelOpen().then([=]() {
            const QString queue = "hello";
            const QString msg = "Hello world";
            channel->queueDeclare(queue).then([channel, msg, queue](const QVariantList &ret) {
                channel->basicPublish(qmq::Message(msg.toUtf8(), QString(), queue));
                qInfo() << " [x] Sent" << msg;
            });
        });
    });

    return app.exec();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("Hello World Client / Server");
    parser.addHelpOption();
    parser.addVersionOption();

    // A boolean option with multiple names (-f, --force)
    const QCommandLineOption serverOption(QStringList() << "s"
                                                        << "server",
                                          QCoreApplication::translate("main", "Start as server."));
    parser.addOption(serverOption);
    // Process the actual command line arguments given by the user
    parser.process(app);

    const bool isServer = parser.isSet(serverOption);
    if (isServer) {
        return receiverMain(app);
    }
    return senderMain(app);
}