/*************************************************************************************
 *  Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
 *                                                                                   *
 *  This program is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU General Public License                      *
 *  as published by the Free Software Foundation; either version 2                   *
 *  of the License, or (at your option) any later version.                           *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *                                                                                   *
 *  You should have received a copy of the GNU General Public License                *
 *  along with this program; if not, write to the Free Software                      *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA   *
 *************************************************************************************/

#include "daemon.h"
#include "serializer.h"
#include "generator.h"

#include <QtCore/QDebug>

#include <kdemacros.h>
#include <KPluginFactory>

#include <kscreen/config.h>
#include <kscreen/configmonitor.h>
#include <kaction.h>

K_PLUGIN_FACTORY(KScreenDaemonFactory, registerPlugin<KScreenDaemon>();)
K_EXPORT_PLUGIN(KScreenDaemonFactory("kscreen", "kscreen"))

KScreenDaemon::KScreenDaemon(QObject* parent, const QList< QVariant >& )
 : KDEDModule(parent)
 , m_iteration(0)
 , m_pendingSave(false)
{
    setenv("KSCREEN_BACKEND", "XRandR", 1);
    KAction* action = new KAction(this);
    action->setGlobalShortcut(KShortcut(Qt::Key_Display));

    connect(action, SIGNAL(triggered(bool)), SLOT(displayButton()));
    connect(Generator::self(), SIGNAL(ready()), SLOT(init()));
}

KScreenDaemon::~KScreenDaemon()
{
    Generator::destroy();
}

void KScreenDaemon::init()
{
    applyConfig();
    monitorForChanges();
}

void KScreenDaemon::applyConfig()
{
    qDebug() << "Applying config";
    KScreen::Config* config = 0;
    if (Serializer::configExists()) {
        config = Serializer::config(Serializer::currentId());
    } else {
        config = Generator::self()->idealConfig();
    }

    KScreen::Config::setConfig(config);
}

void KScreenDaemon::configChanged()
{
    qDebug() << "Change detected";
    if (m_pendingSave) {
        return;
    }

    qDebug() << "Scheduling screen save";
    m_pendingSave = true;
    QMetaObject::invokeMethod(this, "saveCurrentConfig", Qt::QueuedConnection);
}

void KScreenDaemon::saveCurrentConfig()
{
    qDebug() << "Saving current config";
    m_pendingSave = false;
    Serializer::saveConfig(KScreen::Config::current());
}

void KScreenDaemon::displayButton()
{
    if (m_iteration > 5) {
        m_iteration = 1;
    }

    Generator::self()->displaySwitch(m_iteration);
}

void KScreenDaemon::monitorForChanges()
{
    KScreen::Config* config = KScreen::Config::current();
    KScreen::ConfigMonitor::instance()->addConfig(config);

    KScreen::OutputList outputs = config->outputs();
    Q_FOREACH(KScreen::Output* output, outputs) {
        connect(output, SIGNAL(isConnectedChanged()), SLOT(applyConfig()));

        connect(output, SIGNAL(currentModeChanged()), SLOT(configChanged()));
        connect(output, SIGNAL(isEnabledChanged()), SLOT(configChanged()));
        connect(output, SIGNAL(isPrimaryChanged()), SLOT(configChanged()));
        connect(output, SIGNAL(outputChanged()), SLOT(configChanged()));
        connect(output, SIGNAL(clonesChanged()), SLOT(configChanged()));
        connect(output, SIGNAL(posChanged()), SLOT(configChanged()));
        connect(output, SIGNAL(rotationChanged()), SLOT(configChanged()));
    }
}