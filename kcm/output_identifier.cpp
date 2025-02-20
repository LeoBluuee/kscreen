/*
    SPDX-FileCopyrightText: 2019 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "output_identifier.h"

#include "../common/utils.h"

#include <kscreen/edid.h>
#include <kscreen/output.h>

#include <QQuickItem>
#include <QStandardPaths>
#include <QTimer>

#include <KDeclarative/kdeclarative/qmlobject.h>
#include <PlasmaQuick/Dialog>

#define QML_PATH "kpackage/kcms/kcm_kscreen/contents/ui/"

OutputIdentifier::OutputIdentifier(KScreen::ConfigPtr config, QObject *parent)
    : QObject(parent)
{
    const QString qmlPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral(QML_PATH "OutputIdentifier.qml"));

    const auto outputs = config->connectedOutputs();
    for (const auto &output : outputs) {
        if (!output->currentMode()) {
            continue;
        }

        const KScreen::ModePtr mode = output->currentMode();
        auto *view = new PlasmaQuick::Dialog();

        auto qmlObject = new KDeclarative::QmlObject(view);
        qmlObject->setSource(QUrl::fromLocalFile(qmlPath));
        qmlObject->completeInitialization();

        auto rootObj = qobject_cast<QQuickItem *>(qmlObject->rootObject());

        view->setMainItem(rootObj);
        view->setFlags(Qt::X11BypassWindowManagerHint | Qt::FramelessWindowHint);
        view->setBackgroundHints(PlasmaQuick::Dialog::NoBackground);
        view->installEventFilter(this);

        if (!rootObj) {
            delete view;
            continue;
        }

        QSize logicalSize = config->logicalSizeForOutput(*output.data()).toSize();
        if (!(config->supportedFeatures() & KScreen::Config::Feature::PerOutputScaling)) {
            logicalSize = logicalSize / view->effectiveDevicePixelRatio();
        }

        bool shouldShowSerialNumber = false;
        if (output->edid()) {
            shouldShowSerialNumber = std::any_of(outputs.cbegin(), outputs.cend(), [output](const auto &other) {
                return other->id() != output->id() // avoid same output
                    && other->edid() && other->edid()->name() == output->edid()->name() && other->edid()->vendor() == output->edid()->vendor();
            });
        }
        rootObj->setProperty("outputName", Utils::outputName(output, shouldShowSerialNumber));
        rootObj->setProperty("resolution", output->currentMode()->size());
        rootObj->setProperty("scale", output->scale());
        view->setProperty("screenSize", QRect(output->pos(), logicalSize));
        m_views << view;
    }

    for (auto *view : qAsConst(m_views)) {
        view->show();
    }
    QTimer::singleShot(2500, this, &OutputIdentifier::identifiersFinished);
}

OutputIdentifier::~OutputIdentifier()
{
    qDeleteAll(m_views);
}

bool OutputIdentifier::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        if (m_views.contains(qobject_cast<PlasmaQuick::Dialog *>(object))) {
            QResizeEvent *e = static_cast<QResizeEvent *>(event);
            const QRect screenSize = object->property("screenSize").toRect();
            QRect geometry(QPoint(0, 0), e->size());
            geometry.moveCenter(screenSize.center());
            static_cast<PlasmaQuick::Dialog *>(object)->setGeometry(geometry);
        }
    }
    return QObject::eventFilter(object, event);
}
