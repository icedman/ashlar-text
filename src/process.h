#ifndef PROCESS_H
#define PROCESS_H

#include <QObject>
#include <QProcess>

class Process : public QObject {
    Q_OBJECT
public:
    Process(QObject* parent);

    void init(QString command);
    void run(QStringList args);
    void write(QString input);

    QString program() { return command; }
    bool isAvailable() { return process == 0; }

private Q_SLOTS:

    void readyReadStandardOutput();
    void readyReadStandardError();
    void processExited(int exitCode, QProcess::ExitStatus exitStatus);

signals:

    void output(Process* proc, QString out);
    void finished(Process* proc, int code);

private:
    QProcess* process;
    QString command;
};

#endif