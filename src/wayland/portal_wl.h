/*
 * Copyright (C) 2015-2024 Département de l'Instruction Publique (DIP-SEM)
 *
 * Copyright (C) 2013 Open Education Foundation
 *
 * Copyright (C) 2010-2013 Groupement d'Intérêt Public pour
 * l'Education Numérique en Afrique (GIP ENA)
 *
 * This file is part of OpenBoard.
 *
 * OpenBoard is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License,
 * with a specific linking exception for the OpenSSL project's
 * "OpenSSL" library (or with modified versions of it that use the
 * same license as the "OpenSSL" library).
 *
 * OpenBoard is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBoard. If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once

#include <QObject>
#include <QRect>
#include <QVariantMap>

// forward
class QDBusInterface;
class QScreen;

class Portal_wl : public QObject
{
    Q_OBJECT

public:
    typedef struct
    {
        uint node_id;
        QVariantMap map;
    } Stream;
    typedef QList<Stream> Streams;

    explicit Portal_wl(QObject* parent = nullptr);
    virtual ~Portal_wl();

//    void grabScreen(QScreen* screen, const QRect& rect = {});

public slots:
    void startScreenCast(bool withCursor);
    void stopScreenCast();

signals:
    void screenGrabbed(QPixmap pixmap);
    void streamStarted(qint64 fd, QString path);
    void screenCastAborted();

private slots:
//    void handleScreenshotResponse(uint code, const QMap<QString, QVariant>& results);
    void handleCreateSessionResponse(uint response, const QVariantMap& results);
    void handleSelectSourcesResponse(uint response, const QVariantMap& results);
    void handleStartResponse(uint response, const QVariantMap& results);

private:
    QDBusInterface* screencastPortal();
    QString createSessionToken() const;
    QString createRequestToken() const;

private:
    QRect mScreenRect;
    bool mWithCursor{false};
    QString mSession;
    QString mRequestPath;
    QString mRestoreToken;
    QDBusInterface* mScreencastPortal{nullptr};
};
