#include "HttpFileuploader.h"
#include "FileWithCypher.h"

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>


HttpFileUploader::HttpFileUploader(QObject *parent) : QObject(parent),
    networkManager_(new QNetworkAccessManager(parent)), request_(new QNetworkRequest()),
    file_(nullptr)
{
    connect(networkManager_, SIGNAL(finished(QNetworkReply*) ), this, SLOT(putFinished(QNetworkReply*)));
}

HttpFileUploader::~HttpFileUploader()
{
    delete request_;
}

void HttpFileUploader::upload(QString url, FileWithCypher* file)
{
    file_ = file;

    if (!file->exists())
    {
        qDebug() << "error on file. " << file->fileName();
        emit errorOccurred();
        return;
    }

    if (file->open(QIODevice::ReadOnly) == true)
    {
        QUrl theUrl(url);

        qDebug() << "url valid: " << theUrl.isValid() << ", host: " << theUrl.host() <<
                    ", path: " << theUrl.path() << ", port:" << theUrl.port();

        request_->setUrl(theUrl);

        QNetworkReply *reply = networkManager_->put(*request_, file);

        if (reply != nullptr)
        {
           //qDebug() << "upload runnging? " << reply->isRunning() << ", error: " << reply->errorString();

            connect(reply, SIGNAL(uploadProgress(qint64, qint64)),
                    this, SLOT(displayProgress(qint64, qint64)));

            connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
                    this, SLOT(error(QNetworkReply::NetworkError)));

            connect(reply, SIGNAL(finished()), this, SLOT(finished()));
        }
        else
        {
            qDebug() << "error creating QNetworkReply Object";
            emit errorOccurred();
        }
    }
    else
    {
        qDebug() << "file open error";
        emit errorOccurred();
    }
}

void HttpFileUploader::putFinished(QNetworkReply* reply)
{
    QByteArray response = reply->readAll();

    //printf("response: %s\n", response.data() );
    //printf("reply error %d\n", reply->error() );

    if (file_ != nullptr && file_->isOpen())
    {
        file_->close();
    }

    if (reply->error() == QNetworkReply::NoError)
    {
        emit updateStatus("done");
        emit uploadSuccess(file_->getIvAndKey());
    }
    else
    {
        emit errorOccurred();
    }

    file_ = nullptr;
}

void HttpFileUploader::finished()
{
    qDebug() << "finished";

    emit updateStatus("done");
}

void HttpFileUploader::error(QNetworkReply::NetworkError code)
{
    // code 0 is success
    qDebug() << "error code: " << code;
    QString status = "error: " + QString::number(code);
    emit updateStatus(status);
}

void HttpFileUploader::displayProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    qDebug() << "progress: " << bytesReceived << "/" << bytesTotal;

    QString status = bytesTotal > 0 ? QString::number((bytesReceived*100) / bytesTotal)+"%" : "";

    emit updateStatus(status);
}
