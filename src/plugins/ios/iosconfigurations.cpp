/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "iosconfigurations.h"
#include "iosconstants.h"
#include "iosdevice.h"
#include "iossimulator.h"
#include "iosprobe.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggerkitinformation.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtversionfactory.h>

#include <QFileInfo>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QStringList>
#include <QTimer>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;
using namespace Debugger;

const bool debugProbe = false;

namespace Ios {
namespace Internal {

const QLatin1String SettingsGroup("IosConfigurations");
const QLatin1String ignoreAllDevicesKey("IgnoreAllDevices");

void IosConfigurations::updateAutomaticKitList()
{
    QMap<QString, Platform> platforms = IosProbe::detectPlatforms();
    {
        QMapIterator<QString, Platform> iter(platforms);
        while (iter.hasNext()) {
            iter.next();
            const Platform &p = iter.value();
            setDeveloperPath(p.developerPath);
            break;
        }
    }
    // filter out all non iphone, non base, non clang or cxx11 platforms, as we don't set up kits for those
    {
        QMap<QString, Platform>::iterator iter(platforms.begin());
        while (iter != platforms.end()) {
            const Platform &p = iter.value();
            if (!p.name.startsWith(QLatin1String("iphone")) || (p.platformKind & Platform::BasePlatform) == 0
                    || (p.platformKind & Platform::Cxx11Support) != 0
                    || !p.compilerPath.toString().contains(QLatin1String("clang")))
                iter = platforms.erase(iter);
            else {
                if (debugProbe)
                    qDebug() << "keeping" << p.name << " " << p.compilerPath.toString() << " "
                             << p.backendFlags;
                ++iter;
            }
        }
    }

    QMap<QString, GccToolChain *> platformToolchainMap;
    // check existing toolchains (and remove old ones)
    foreach (ToolChain *tc, ToolChainManager::toolChains()) {
        if (!tc->isAutoDetected()) // use also user toolchains?
            continue;
        if (tc->type() != QLatin1String("clang") && tc->type() != QLatin1String("gcc"))
            continue;
        GccToolChain *toolchain = static_cast<GccToolChain *>(tc);
        QMapIterator<QString, Platform> iter(platforms);
        bool found = false;
        while (iter.hasNext()) {
            iter.next();
            const Platform &p = iter.value();
            if (p.compilerPath == toolchain->compilerCommand()
                    && p.backendFlags == toolchain->platformCodeGenFlags()
                    && !platformToolchainMap.contains(p.name)) {
                platformToolchainMap[p.name] = toolchain;
                found = true;
            }
        }
        iter.toFront();
        while (iter.hasNext()) {
            iter.next();
            const Platform &p = iter.value();
            if (p.platformKind)
                continue;
            if (p.compilerPath == toolchain->compilerCommand()
                    && p.backendFlags == toolchain->platformCodeGenFlags()) {
                found = true;
                if (p.architecture == QLatin1String("i386")
                        && toolchain->targetAbi().wordWidth() != 32) {
                    if (debugProbe)
                        qDebug() << "resetting api of " << toolchain->displayName();
                    toolchain->setTargetAbi(Abi(Abi::X86Architecture,
                                                Abi::MacOS, Abi::GenericMacFlavor,
                                                Abi::MachOFormat, 32));
                }
                platformToolchainMap[p.name] = toolchain;
                if (debugProbe)
                    qDebug() << p.name << " -> " << toolchain->displayName();
            }
        }
        if (!found && (tc->displayName().startsWith(QLatin1String("iphone"))
                       || tc->displayName().startsWith(QLatin1String("mac")))) {
            qDebug() << "removing toolchain" << tc->displayName();
            ToolChainManager::deregisterToolChain(tc);
        }
    }
    // add missing toolchains
    {
        QMapIterator<QString, Platform> iter(platforms);
        while (iter.hasNext()) {
            iter.next();
            const Platform &p = iter.value();
            if (platformToolchainMap.contains(p.name))
                continue;
            GccToolChain *toolchain;
            if (p.compilerPath.toFileInfo().baseName().startsWith(QLatin1String("clang")))
                toolchain = new ClangToolChain(ToolChain::AutoDetection);
            else
                toolchain = new GccToolChain(
                            QLatin1String(ProjectExplorer::Constants::GCC_TOOLCHAIN_ID),
                            ToolChain::AutoDetection);
            QString baseDisplayName = p.name;
            QString displayName = baseDisplayName;
            for (int iVers = 1; iVers < 100; ++iVers) {
                bool unique = true;
                foreach (ToolChain *existingTC, ToolChainManager::toolChains()) {
                    if (existingTC->displayName() == displayName) {
                        unique = false;
                        break;
                    }
                }
                if (unique) break;
                displayName = baseDisplayName + QLatin1String("-") + QString::number(iVers);
            }
            toolchain->setDisplayName(displayName);
            toolchain->setPlatformCodeGenFlags(p.backendFlags);
            toolchain->setPlatformLinkerFlags(p.backendFlags);
            toolchain->setCompilerCommand(p.compilerPath);
            if (p.architecture == QLatin1String("i386")) {
                if (debugProbe)
                    qDebug() << "setting toolchain Abi for " << toolchain->displayName();
                toolchain->setTargetAbi(Abi(Abi::X86Architecture,Abi::MacOS, Abi::GenericMacFlavor,
                                            Abi::MachOFormat, 32));
            }
            if (debugProbe)
                qDebug() << "adding toolchain " << p.name;
            ToolChainManager::registerToolChain(toolchain);
            platformToolchainMap.insert(p.name, toolchain);
            QMapIterator<QString, Platform> iter2(iter);
            while (iter2.hasNext()) {
                iter2.next();
                const Platform &p2 = iter2.value();
                if (!platformToolchainMap.contains(p2.name)
                        && p2.compilerPath == toolchain->compilerCommand()
                        && p2.backendFlags == toolchain->platformCodeGenFlags()) {
                    platformToolchainMap[p2.name] = toolchain;
                }
            }
        }
    }
    QMap<Abi::Architecture, QList<BaseQtVersion *> > qtVersionsForArch;
    foreach (BaseQtVersion *qtVersion, QtVersionManager::versions()) {
        if (debugProbe)
            qDebug() << "qt type " << qtVersion->type();
        if (qtVersion->type() != QLatin1String(Constants::IOSQT)) {
            if (qtVersion->qmakeProperty("QMAKE_PLATFORM").contains(QLatin1String("ios"))
                    || qtVersion->qmakeProperty("QMAKE_XSPEC").contains(QLatin1String("ios"))) {
                // replace with an ios version
                BaseQtVersion *iosVersion =
                        QtVersionFactory::createQtVersionFromQMakePath(
                            qtVersion->qmakeCommand(),
                            qtVersion->isAutodetected(),
                            qtVersion->autodetectionSource());
                if (iosVersion && iosVersion->type() == QLatin1String(Constants::IOSQT)) {
                    if (debugProbe)
                        qDebug() << "converting QT to iOS QT for " << qtVersion->qmakeCommand().toUserOutput();
                    QtVersionManager::removeVersion(qtVersion);
                    QtVersionManager::addVersion(iosVersion);
                    qtVersion = iosVersion;
                } else {
                    continue;
                }
            } else {
                continue;
            }
        }
        if (!qtVersion->isValid())
            continue;
        QList<Abi> qtAbis = qtVersion->qtAbis();
        if (qtAbis.empty())
            continue;
        if (debugProbe)
            qDebug() << "qt arch " << qtAbis.first().architecture();
        foreach (const Abi &abi, qtAbis)
            qtVersionsForArch[abi.architecture()].append(qtVersion);
    }

    const DebuggerItem *possibleDebugger = DebuggerItemManager::findByEngineType(Debugger::LldbEngineType);
    QVariant debuggerId = (possibleDebugger ? possibleDebugger->id() : QVariant());

    QList<Kit *> existingKits;
    QList<bool> kitMatched;
    foreach (Kit *k, KitManager::kits()) {
        Core::Id deviceKind = DeviceTypeKitInformation::deviceTypeId(k);
        if (deviceKind != Constants::IOS_DEVICE_TYPE
                && deviceKind != Constants::IOS_SIMULATOR_TYPE) {
            if (debugProbe)
                qDebug() << "skipping existing kit with deviceKind " << deviceKind.toString();
            continue;
        }
        if (!k->isAutoDetected())
            continue;
        existingKits << k;
        kitMatched << false;
    }
    // create missing kits
    {
        QMapIterator<QString, Platform> iter(platforms);
        while (iter.hasNext()) {
            iter.next();
            const Platform &p = iter.value();
            GccToolChain *pToolchain = platformToolchainMap.value(p.name, 0);
            if (!pToolchain)
                continue;
            Core::Id pDeviceType;
            if (debugProbe)
                qDebug() << "guaranteeing kit for " << p.name ;
            if (p.name.startsWith(QLatin1String("iphoneos-"))) {
                pDeviceType = Constants::IOS_DEVICE_TYPE;
            } else if (p.name.startsWith(QLatin1String("iphonesimulator-"))) {
                pDeviceType = Constants::IOS_SIMULATOR_TYPE;
                if (debugProbe)
                    qDebug() << "pDeviceType " << pDeviceType.toString();
            } else {
                if (debugProbe)
                    qDebug() << "skipping non ios kit " << p.name;
                // we looked up only the ios qt build above...
                continue;
                //pDeviceType = Constants::DESKTOP_DEVICE_TYPE;
            }
            Abi::Architecture arch = pToolchain->targetAbi().architecture();

            QList<BaseQtVersion *> qtVersions = qtVersionsForArch.value(arch);
            foreach (BaseQtVersion *qt, qtVersions) {
                Kit *kitAtt = 0;
                bool kitExists = false;
                for (int i = 0; i < existingKits.size(); ++i) {
                    Kit *k = existingKits.at(i);
                    if (DeviceTypeKitInformation::deviceTypeId(k) == pDeviceType
                            && ToolChainKitInformation::toolChain(k) == pToolchain
                            && QtKitInformation::qtVersion(k) == qt)
                    {
                        QTC_CHECK(!kitMatched.value(i, true));
                        // as we generate only two kits per qt (one for device and one for simulator)
                        // we do not compare the sdk (thus automatically upgrading it in place if a
                        // new Xcode is used). Change?
                        kitExists = true;
                        kitAtt = k;
                        if (debugProbe)
                            qDebug() << "found existing kit " << k->displayName() << " for " << p.name
                                     << "," << qt->displayName();
                        if (i<kitMatched.size())
                            kitMatched.replace(i, true);
                        break;
                    }
                }
                if (kitExists) {
                    kitAtt->blockNotification();
                } else {
                    if (debugProbe)
                        qDebug() << "setting up new kit for " << p.name;
                    kitAtt = new Kit;
                    kitAtt->setAutoDetected(true);
                    QString baseDisplayName = tr("%1 %2").arg(p.name, qt->displayName());
                    QString displayName = baseDisplayName;
                    for (int iVers = 1; iVers < 100; ++iVers) {
                        bool unique = true;
                        foreach (const Kit *k, existingKits) {
                            if (k->displayName() == displayName) {
                                unique = false;
                                break;
                            }
                        }
                        if (unique) break;
                        displayName = baseDisplayName + QLatin1String("-") + QString::number(iVers);
                    }
                    kitAtt->setDisplayName(displayName);
                }
                kitAtt->setIconPath(Utils::FileName::fromString(
                                        QLatin1String(Constants::IOS_SETTINGS_CATEGORY_ICON)));
                DeviceTypeKitInformation::setDeviceTypeId(kitAtt, pDeviceType);
                ToolChainKitInformation::setToolChain(kitAtt, pToolchain);
                QtKitInformation::setQtVersion(kitAtt, qt);
                if ((!Debugger::DebuggerKitInformation::debugger(kitAtt)
                        || !Debugger::DebuggerKitInformation::debugger(kitAtt)->isValid()
                        || Debugger::DebuggerKitInformation::debugger(kitAtt)->engineType() != Debugger::LldbEngineType)
                        && debuggerId.isValid())
                    Debugger::DebuggerKitInformation::setDebugger(kitAtt,
                                                                  debuggerId);

                kitAtt->setMutable(DeviceKitInformation::id(), true);
                kitAtt->setSticky(QtKitInformation::id(), true);
                kitAtt->setSticky(ToolChainKitInformation::id(), true);
                kitAtt->setSticky(DeviceTypeKitInformation::id(), true);
                kitAtt->setSticky(SysRootKitInformation::id(), true);
                kitAtt->setSticky(Debugger::DebuggerKitInformation::id(), false);

                SysRootKitInformation::setSysRoot(kitAtt, p.sdkPath);
                // QmakeProjectManager::QmakeKitInformation::setMkspec(newKit,
                //    Utils::FileName::fromLatin1("macx-ios-clang")));
                if (kitExists) {
                    kitAtt->unblockNotification();
                } else {
                    KitManager::registerKit(kitAtt);
                    existingKits << kitAtt;
                }
            }
        }
    }
    for (int i = 0; i < kitMatched.size(); ++i) {
        // deleting extra (old) kits
        if (!kitMatched.at(i)) {
            qDebug() << "deleting kit " << existingKits.at(i)->displayName();
            KitManager::deregisterKit(existingKits.at(i));
        }
    }
}

static IosConfigurations *m_instance = 0;

QObject *IosConfigurations::instance()
{
    return m_instance;
}

void IosConfigurations::initialize()
{
    QTC_CHECK(m_instance == 0);
    m_instance = new IosConfigurations(0);
    m_instance->updateSimulators();
    QTimer::singleShot(10000, IosDeviceManager::instance(), SLOT(monitorAvailableDevices()));
}

bool IosConfigurations::ignoreAllDevices()
{
    return m_instance->m_ignoreAllDevices;
}

void IosConfigurations::setIgnoreAllDevices(bool ignoreDevices)
{
    if (ignoreDevices != m_instance->m_ignoreAllDevices) {
        m_instance->m_ignoreAllDevices = ignoreDevices;
        m_instance->save();
        emit m_instance->updated();
    }
}

FileName IosConfigurations::developerPath()
{
    return m_instance->m_developerPath;
}

void IosConfigurations::save()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    settings->setValue(ignoreAllDevicesKey, m_ignoreAllDevices);
    settings->endGroup();
}

IosConfigurations::IosConfigurations(QObject *parent)
    : QObject(parent)
{
    load();
}

void IosConfigurations::load()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    m_ignoreAllDevices = settings->value(ignoreAllDevicesKey, false).toBool();
    settings->endGroup();
}

void IosConfigurations::updateSimulators()
{
    // currently we have just one simulator
    DeviceManager *devManager = DeviceManager::instance();
    Core::Id devId = Constants::IOS_SIMULATOR_DEVICE_ID;
    IDevice::ConstPtr dev = devManager->find(devId);
    if (!dev.isNull())
        return;
    IosSimulator *newDev = new IosSimulator(devId);
    devManager->addDevice(IDevice::ConstPtr(newDev));
}

void IosConfigurations::setDeveloperPath(const FileName &devPath)
{
    if (devPath != m_instance->m_developerPath) {
        m_instance->m_developerPath = devPath;
        m_instance->save();
        updateAutomaticKitList();
        emit m_instance->updated();
    }
}

} // namespace Internal
} // namespace Ios
