#include "process.h"

#include <QDebug>

Process::Process(QObject* parent)
    : QObject(parent)
    , process(0)
{
}

void Process::init(QString cmd)
{
    if (process) {
        return;
    }

    command = cmd;
    process = new QProcess(this);
    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readyReadStandardOutput()));
    connect(process, SIGNAL(readyReadStandardError()), this, SLOT(readyReadStandardError()));
    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processExited(int, QProcess::ExitStatus)));
}

void Process::write(QString input)
{
    if (process) {
        process->write((const char*)input.data(), input.size());
    }
}

void Process::run(QStringList args)
{
    if (!process) {
        return;
    }

    process->start(command, args);
}

void Process::readyReadStandardOutput()
{
    QString out = process->readAllStandardOutput();
    out += "\n";
    emit output(this, out);
}

void Process::readyReadStandardError()
{
    QString out = process->readAllStandardError();
    out += "\n";
    qDebug() << out;
    emit output(this, out);
}

void Process::processExited(int exitCode, QProcess::ExitStatus exitStatus)
{
    process->deleteLater();
    process = 0;
    emit finished(this, exitCode);
}
