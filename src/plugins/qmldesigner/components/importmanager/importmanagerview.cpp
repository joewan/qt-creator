/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "importmanagerview.h"

namespace QmlDesigner {

ImportManagerView::ImportManagerView(QObject *parent) :
    AbstractView(parent)
{
}

void ImportManagerView::modelAttached(Model *model)
{

}

void ImportManagerView::modelAboutToBeDetached(Model *model)
{

}

void ImportManagerView::nodeCreated(const ModelNode &createdNode)
{
// handle case that the import is used
}

void ImportManagerView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
// handle case that the import is unused
}

void ImportManagerView::nodeRemoved(const ModelNode &/*removedNode*/, const NodeAbstractProperty &/*parentProperty*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{

}

void ImportManagerView::nodeAboutToBeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{

}

void ImportManagerView::nodeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{

}

void ImportManagerView::nodeIdChanged(const ModelNode &/*node*/, const QString &/*newId*/, const QString &/*oldId*/)
{

}

void ImportManagerView::propertiesAboutToBeRemoved(const QList<AbstractProperty> &/*propertyList*/)
{

}

void ImportManagerView::propertiesRemoved(const QList<AbstractProperty> &/*propertyList*/)
{

}

void ImportManagerView::variantPropertiesChanged(const QList<VariantProperty> &/*propertyList*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{

}

void ImportManagerView::bindingPropertiesChanged(const QList<BindingProperty> &/*propertyList*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{

}

void ImportManagerView::signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &/*propertyList*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{

}

void ImportManagerView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{

}

void ImportManagerView::instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &/*propertyList*/)
{

}

void ImportManagerView::instancesCompleted(const QVector<ModelNode> &/*completedNodeList*/)
{

}

void ImportManagerView::instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &/*informationChangeHash*/)
{

}

void ImportManagerView::instancesRenderImageChanged(const QVector<ModelNode> &/*nodeList*/)
{

}

void ImportManagerView::instancesPreviewImageChanged(const QVector<ModelNode> &/*nodeList*/)
{

}

void ImportManagerView::instancesChildrenChanged(const QVector<ModelNode> &/*nodeList*/)
{

}

void ImportManagerView::instancesToken(const QString &/*tokenName*/, int /*tokenNumber*/, const QVector<ModelNode> &/*nodeVector*/)
{

}

void ImportManagerView::nodeSourceChanged(const ModelNode &/*modelNode*/, const QString &/*newNodeSource*/)
{

}

void ImportManagerView::rewriterBeginTransaction()
{

}

void ImportManagerView::rewriterEndTransaction()
{

}

void ImportManagerView::currentStateChanged(const ModelNode &/*node*/)
{

}

void ImportManagerView::selectedNodesChanged(const QList<ModelNode> &/*selectedNodeList*/, const QList<ModelNode> &/*lastSelectedNodeList*/)
{

}

void ImportManagerView::fileUrlChanged(const QUrl &/*oldUrl*/, const QUrl &/*newUrl*/)
{

}

void ImportManagerView::nodeOrderChanged(const NodeListProperty &/*listProperty*/, const ModelNode &/*movedNode*/, int /*oldIndex*/)
{

}

void ImportManagerView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{

}

void ImportManagerView::auxiliaryDataChanged(const ModelNode &/*node*/, const PropertyName &/*name*/, const QVariant &/*data*/)
{

}

void ImportManagerView::customNotification(const AbstractView */*view*/, const QString &/*identifier*/, const QList<ModelNode> &/*nodeList*/, const QList<QVariant> &/*data*/)
{

}

void ImportManagerView::scriptFunctionsChanged(const ModelNode &/*node*/, const QStringList &/*scriptFunctionList*/)
{

}

} // namespace QmlDesigner