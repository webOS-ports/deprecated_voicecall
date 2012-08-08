/*
 * This file is a part of the Voice Call Manager project
 *
 * Copyright (C) 2011-2012  Tom Swindell <t.swindell@rubyx.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
#include "common.h"
#include "voicecallmanager.h"

#include <QHash>
#include <QUuid>

class VoiceCallManagerPrivate
{
    Q_DECLARE_PUBLIC(VoiceCallManager)

public:
    VoiceCallManagerPrivate(VoiceCallManager *q)
        : q_ptr(q), activeVoiceCall(NULL), muteMicrophone(false), muteRingtone(false)
    {/* ... */}

    VoiceCallManager *q_ptr;

    QHash<QString, AbstractVoiceCallProvider*> providers;

    AbstractVoiceCallHandler *activeVoiceCall;

    bool muteMicrophone;
    bool muteRingtone;

    QString errorString;
};

VoiceCallManager::VoiceCallManager(QObject *parent)
    : VoiceCallManagerInterface(parent), d_ptr(new VoiceCallManagerPrivate(this))
{
    TRACE
}

VoiceCallManager::~VoiceCallManager()
{
    TRACE
    Q_D(VoiceCallManager);
    delete d;
}

QString VoiceCallManager::errorString() const
{
    TRACE
    Q_D(const VoiceCallManager);
    return d->errorString;
}

void VoiceCallManager::setError(const QString &errorString)
{
    TRACE
    Q_D(VoiceCallManager);
    d->errorString = errorString;
    qDebug() << d->errorString;
    emit this->error(d->errorString);
}

AbstractVoiceCallHandler* VoiceCallManager::activeVoiceCall() const
{
    TRACE
    Q_D(const VoiceCallManager);
    return d->activeVoiceCall;
}

QList<AbstractVoiceCallProvider*> VoiceCallManager::providers() const
{
    TRACE
    Q_D(const VoiceCallManager);
    return d->providers.values();
}

void VoiceCallManager::appendProvider(AbstractVoiceCallProvider *provider)
{
    TRACE
    Q_D(VoiceCallManager);
    if(d->providers.contains(provider->providerId())) return;

    DEBUG_T(QString("VCM: Registering voice call provider: ") + provider->providerId());
    QObject::connect(provider,
                     SIGNAL(voiceCallsChanged()),
                     SIGNAL(voiceCallsChanged()));
    QObject::connect(provider,
                     SIGNAL(voiceCallAdded(AbstractVoiceCallHandler*)),
                     SLOT(onVoiceCallAdded(AbstractVoiceCallHandler*)));
    QObject::connect(provider,
                     SIGNAL(voiceCallRemoved(QString)),
                     SLOT(onVoiceCallRemoved(QString)));

    d->providers.insert(provider->providerId(), provider);
    emit this->providersChanged();
    emit this->providerAdded(provider);
}

void VoiceCallManager::removeProvider(AbstractVoiceCallProvider *provider)
{
    TRACE
    Q_D(VoiceCallManager);
    if(!d->providers.contains(provider->providerId())) return;

    DEBUG_T(QString("VCM: Deregistering voice call provider: ") + provider->providerId());
    QObject::disconnect(provider,
                        SIGNAL(voiceCallsChanged()),
                        this,
                        SIGNAL(voiceCallsChanged()));
    QObject::disconnect(provider,
                        SIGNAL(voiceCallAdded(AbstractVoiceCallHandler*)),
                        this,
                        SLOT(onVoiceCallAdded(AbstractVoiceCallHandler*)));
    QObject::disconnect(provider,
                        SIGNAL(voiceCallRemoved(QString)),
                        this,
                        SLOT(onVoiceCallRemoved(QString)));

    d->providers.remove(provider->providerId());
    emit this->providersChanged();
    emit this->providerRemoved(provider->providerId());
}

QString VoiceCallManager::generateHandlerId()
{
    TRACE
    QString handlerId = QUuid::createUuid().toString().mid(1, 36); // Remove curly braces.
    return handlerId.replace('-', ""); // Remove dashes, (can't have dashes in dbus paths)
}

int VoiceCallManager::voiceCallCount() const
{
    TRACE
    Q_D(const VoiceCallManager);
    int result = 0;

    foreach(AbstractVoiceCallProvider *provider, d->providers.values())
    {
        result += provider->voiceCalls().length();
    }

    return result;
}

QList<AbstractVoiceCallHandler*> VoiceCallManager::voiceCalls() const
{
    TRACE
    Q_D(const VoiceCallManager);
    QList<AbstractVoiceCallHandler*> results;

    foreach(AbstractVoiceCallProvider *provider, d->providers)
    {
        results.append(provider->voiceCalls());
    }

    return results;
}

bool VoiceCallManager::muteMicrophone() const
{
    TRACE
    Q_D(const VoiceCallManager);
    return d->muteMicrophone;
}

bool VoiceCallManager::muteRingtone() const
{
    TRACE
    Q_D(const VoiceCallManager);
    return d->muteRingtone;
}

void VoiceCallManager::setMuteMicrophone(bool on)
{
    TRACE
    Q_D(VoiceCallManager);
    d->muteMicrophone = on;
    emit this->setMuteMicrophoneRequested(on);
}

void VoiceCallManager::setMuteRingtone(bool on)
{
    TRACE
    Q_D(VoiceCallManager);
    d->muteRingtone = on;
    emit this->setMuteRingtoneRequested(on);
}

bool VoiceCallManager::dial(const QString &providerId, const QString &msisdn)
{
    TRACE
    Q_D(VoiceCallManager);
    AbstractVoiceCallProvider *provider = d->providers.value(providerId);

    if(!provider)
    {
        this->setError(QString("*** Unable to find voice call provider with id: ") + providerId);
        return false;
    }

    if(!provider->dial(msisdn))
    {
        this->setError(provider->errorString());
        return false;
    }

    return true;
}

void VoiceCallManager::startEventTone(ToneType type, int volume)
{
    TRACE
    emit this->startEventToneRequested(type, volume);
}

void VoiceCallManager::stopEventTone()
{
    TRACE
    emit this->stopEventToneRequested();
}

void VoiceCallManager::startDtmfTone(const QString &tone, int volume)
{
    TRACE
    emit this->startDtmfToneRequested(tone, volume);
}

void VoiceCallManager::stopDtmfTone()
{
    TRACE
    emit this->stopDtmfToneRequested();
}

void VoiceCallManager::onVoiceCallAdded(AbstractVoiceCallHandler *handler)
{
    TRACE
    Q_D(VoiceCallManager);

    emit this->voiceCallAdded(handler);
    emit this->voiceCallsChanged();

    if(!d->activeVoiceCall)
    {
        d->activeVoiceCall = handler;
        emit this->activeVoiceCallChanged();
    }
}

void VoiceCallManager::onVoiceCallRemoved(const QString &handlerId)
{
    TRACE
    Q_D(VoiceCallManager);

    emit this->voiceCallRemoved(handlerId);
    emit this->voiceCallsChanged();

    if(d->activeVoiceCall->handlerId() == handlerId)
    {
        d->activeVoiceCall = NULL;
        emit this->activeVoiceCallChanged();
    }
}
