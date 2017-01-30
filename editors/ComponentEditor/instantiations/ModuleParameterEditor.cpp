//-----------------------------------------------------------------------------
// File: ModuleParameterEditor.cpp
//-----------------------------------------------------------------------------
// Project: Kactus 2
// Author: Esko Pekkarinen
// Date: 19.02.2015
//
// Description:
// Editor for module parameters.
//-----------------------------------------------------------------------------

#include "ModuleParameterEditor.h"

#include "ModuleParameterModel.h"
#include "ModuleParameterDelegate.h"
#include "ModuleParameterHeaderView.h"

#include <common/views/EditableTableView/editabletableview.h>

#include <editors/ComponentEditor/parameters/ComponentParameterModel.h>

#include "ModuleParameterColumns.h"
#include "ModuleParameterHeaderView.h"

#include <editors/ComponentEditor/common/ComponentParameterFinder.h>
#include <editors/ComponentEditor/common/ExpressionFormatter.h>
#include <editors/ComponentEditor/common/IPXactSystemVerilogParser.h>
#include <editors/ComponentEditor/common/ParameterFinder.h>
#include <editors/ComponentEditor/common/ParameterCompleter.h>

#include <QSortFilterProxyModel>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------
// Function: ModuleParameterEditor::ModuleParameterEditor()
//-----------------------------------------------------------------------------
ModuleParameterEditor::ModuleParameterEditor(QSharedPointer<QList<QSharedPointer<ModuleParameter> > > parameters,
    QSharedPointer<QList<QSharedPointer<Choice> > > componentChoices,
    QSharedPointer<ParameterFinder> parameterFinder,
    QSharedPointer<ExpressionFormatter> expressionFormatter,
    QWidget *parent):
QGroupBox(tr("Module parameters"), parent),
    proxy_(new QSortFilterProxyModel(this)),
    model_(0)
{
    QSharedPointer<IPXactSystemVerilogParser> expressionParser(new IPXactSystemVerilogParser(parameterFinder));

    model_ = new ModuleParameterModel(parameters, componentChoices, expressionParser, parameterFinder,
        expressionFormatter, this);
    
    EditableTableView* view = new EditableTableView(this);

    ModuleParameterHeaderView* horizontalHeader = new ModuleParameterHeaderView(Qt::Horizontal, view);
    view->setHorizontalHeader(horizontalHeader);

    view->verticalHeader()->setMaximumWidth(300);
    view->verticalHeader()->setMinimumWidth(horizontalHeader->fontMetrics().width(tr("Name"))*2);
    view->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    view->verticalHeader()->show();
  
    ComponentParameterModel* parameterModel = new ComponentParameterModel(parameterFinder, this);
    parameterModel->setExpressionParser(expressionParser);

    ParameterCompleter* parameterCompleter = new ParameterCompleter(this);
    parameterCompleter->setModel(parameterModel);

    view->setItemDelegate(new ModuleParameterDelegate(componentChoices, parameterCompleter, parameterFinder,
        expressionFormatter, this));

    connect(model_, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()), Qt::UniqueConnection);
    connect(model_, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
        this, SIGNAL(contentChanged()), Qt::UniqueConnection);
    connect(model_, SIGNAL(errorMessage(const QString&)),
        this, SIGNAL(errorMessage(const QString&)), Qt::UniqueConnection);
    connect(model_, SIGNAL(noticeMessage(const QString&)),
        this, SIGNAL(noticeMessage(const QString&)), Qt::UniqueConnection);

    connect(view, SIGNAL(addItem(const QModelIndex&)), 
        model_, SLOT(onAddItem(const QModelIndex&)), Qt::UniqueConnection);
    connect(view, SIGNAL(removeItem(const QModelIndex&)),
        model_, SLOT(onRemoveItem(const QModelIndex&)), Qt::UniqueConnection);
    connect(view->itemDelegate(), SIGNAL(increaseReferences(QString)), 
        this, SIGNAL(increaseReferences(QString)), Qt::UniqueConnection);
    connect(view->itemDelegate(), SIGNAL(decreaseReferences(QString)),
        this, SIGNAL(decreaseReferences(QString)), Qt::UniqueConnection);

    connect(model_, SIGNAL(decreaseReferences(QString)),
        this, SIGNAL(decreaseReferences(QString)), Qt::UniqueConnection);

    connect(view->itemDelegate(), SIGNAL(openReferenceTree(QString)),
        this, SIGNAL(openReferenceTree(QString)), Qt::UniqueConnection);


    view->setSortingEnabled(true);
    view->setItemsDraggable(false);

    proxy_->setSourceModel(model_);
    view->setModel(proxy_);

    view->sortByColumn(0, Qt::AscendingOrder);
    
    view->setColumnHidden(ModuleParameterColumns::ID, true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(view);
}

//-----------------------------------------------------------------------------
// Function: ModuleParameterEditor::~ModuleParameterEditor()
//-----------------------------------------------------------------------------
ModuleParameterEditor::~ModuleParameterEditor()
{

}

//-----------------------------------------------------------------------------
// Function: ModuleParameterEditor::refresh()
//-----------------------------------------------------------------------------
void ModuleParameterEditor::refresh()
{
    proxy_->invalidate();
}

//-----------------------------------------------------------------------------
// Function: ModuleParameterEditor::enableEditing()
//-----------------------------------------------------------------------------
void ModuleParameterEditor::enableEditing()
{
    model_->enableEditing();
}

//-----------------------------------------------------------------------------
// Function: ModuleParameterEditor::disableEditing()
//-----------------------------------------------------------------------------
void ModuleParameterEditor::disableEditing()
{
    model_->disableEditing();
}

//-----------------------------------------------------------------------------
// Function: ModuleParameterEditor::setModuleParameters()
//-----------------------------------------------------------------------------
void ModuleParameterEditor::setModuleParameters(QSharedPointer<QList<QSharedPointer<ModuleParameter> > > 
    moduleParameters)
{
    model_->setModelParameters(moduleParameters);
}
