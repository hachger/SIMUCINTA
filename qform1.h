#ifndef QFORM1_H
#define QFORM1_H

#include <QMainWindow>
#include "qpaintbox.h"
#include "qserialsetup.h"
#include <QTimer>
#include "UNERBUS.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QMessageBox>

typedef struct{
    QTcpSocket *client;
    uint32_t timeOut;
} _sMyClientList;


QT_BEGIN_NAMESPACE
namespace Ui {
class QForm1;
}
QT_END_NAMESPACE

class QForm1 : public QMainWindow
{
    Q_OBJECT

public:
    QForm1(QWidget *parent = nullptr);
    ~QForm1();

private slots:
    // bool eventFilter(QObject *object, QEvent *event) override;
    void OnQTimer1();
    void OnQSerialPort1();
    // void resizeEvent(QResizeEvent *event) override;

    void OnQTcpServer1Error();
    void OnQTcpServer1ClientConnect();

    void OnQTcpClientTxData();
    void OnQTcpClientDisconnected();

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

private:
    Ui::QForm1 *ui;

    QSerialPort *QSerialPort1;
    QSerialSetup *QSerialSetup1;
    QTimer *QTimer1;
    QPaintBox *QPaintBox1;

    uint8_t rx[256], tx[256];
    _sUNERBUSHandle unerbus;

    void OnDecodeCMD(struct UNERBUSHandle *aBus, uint8_t iStartData);

    QTcpServer *QTcpServer1;
    //    QList<QTcpSocket *> MyTCPClientsList;
    QList<_sMyClientList *> MyTCPClientsList;
};
#endif // QFORM1_H
