//-----------------------------------------------------------------------------
// File: ComponentInstanceLocator.h
//-----------------------------------------------------------------------------
// Project: Kactus2
// Author: Mikko Teuho
// Date: 08.07.2016
//
// Description:
// Locator for component instances within a selected design.
//-----------------------------------------------------------------------------

#include "ComponentInstanceLocator.h"

#include <library/LibraryManager/libraryinterface.h>

#include "ConnectivityConnection.h"
#include "ConnectivityInterface.h"
#include "ConnectivityComponent.h"
#include "MemoryItem.h"
#include "ConnectivityGraph.h"

#include <editors/ComponentEditor/common/SystemVerilogExpressionParser.h>

#include <IPXACTmodels/generaldeclarations.h>
#include <IPXACTmodels/Component/AddressSpace.h>
#include <IPXACTmodels/Component/AddressBlock.h>
#include <IPXACTmodels/Component/BusInterface.h>
#include <IPXACTmodels/Component/Channel.h>
#include <IPXACTmodels/Component/Component.h>
#include <IPXACTmodels/Design/Design.h>
#include <IPXACTmodels/designConfiguration/DesignConfiguration.h>
#include <IPXACTmodels/Component/Field.h>
#include <IPXACTmodels/Component/MasterInterface.h>
#include <IPXACTmodels/Component/MemoryMap.h>
#include <IPXACTmodels/Component/MemoryRemap.h>
#include <IPXACTmodels/Component/RegisterBase.h>
#include <IPXACTmodels/Component/Register.h>
#include <IPXACTmodels/Component/MirroredSlaveInterface.h>

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::ComponentInstanceLocator()
//-----------------------------------------------------------------------------
ComponentInstanceLocator::ComponentInstanceLocator(LibraryInterface* library):
library_(library), expressionParser_(new SystemVerilogExpressionParser())
{

}

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::~ComponentInstanceLocator()
//-----------------------------------------------------------------------------
ComponentInstanceLocator::~ComponentInstanceLocator()
{

}

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::createConnectivityGraph()
//-----------------------------------------------------------------------------
QSharedPointer<ConnectivityGraph> ComponentInstanceLocator::createConnectivityGraph(
    QSharedPointer<const Design> design, QSharedPointer<const DesignConfiguration> designConfiguration)
{
    QSharedPointer<ConnectivityGraph> graph(new ConnectivityGraph());

    if (design)
    {
        analyzeDesign(design, designConfiguration, "", graph);
    }

    return graph;
}

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::analyzeDesign()
//-----------------------------------------------------------------------------
void ComponentInstanceLocator::analyzeDesign(QSharedPointer<const Design> design,
    QSharedPointer<const DesignConfiguration> designConfiguration, QString const& topInstance, 
    QSharedPointer<ConnectivityGraph> graph)
{
    addInstancesAndInterfaces(design, designConfiguration, graph);

    addConnections(design, topInstance, graph);
}

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::addInstancesAndInterfaces()
//-----------------------------------------------------------------------------
void ComponentInstanceLocator::addInstancesAndInterfaces(QSharedPointer<const Design> design, 
    QSharedPointer<const DesignConfiguration> designConfiguration, QSharedPointer<ConnectivityGraph> graph)
{
    foreach (QSharedPointer<ComponentInstance> componentInstance, *design->getComponentInstances())
    {
        QSharedPointer<const Document> instanceDocument = library_->getModelReadOnly(*componentInstance->getComponentRef());
        QSharedPointer<const Component> instancedComponent = instanceDocument.dynamicCast<const Component>();

        if (instancedComponent)
        {
            QString activeView;
            if (designConfiguration)
            {
                activeView = designConfiguration->getActiveView(componentInstance->getInstanceName());
            }

            QSharedPointer<ConnectivityComponent> instanceNode = createInstanceData(componentInstance, 
                instancedComponent, activeView);

            foreach (QSharedPointer<BusInterface> busInterface, *instancedComponent->getBusInterfaces())
            {
                graph->addInterface(createInterfaceData(busInterface, instanceNode));
            }

            foreach (QSharedPointer<Channel> channel, *instancedComponent->getChannels())
            {
                QStringList channelInterfaces = channel->getInterfaces();

                QString startInterfaceName = channelInterfaces.takeFirst();
                QSharedPointer<ConnectivityInterface> startInterface = 
                    graph->getInterface(componentInstance->getInstanceName(), startInterfaceName);

                foreach (QString const& endInterface, channelInterfaces)
                {
                    graph->addConnection(channel->name(), startInterface, 
                        graph->getInterface(componentInstance->getInstanceName(), endInterface));
                }
            }

            graph->addInstance(instanceNode);

            addSubInstances(instanceNode, instancedComponent, designConfiguration, graph);
        }
    }
}

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::getInstanceData()
//-----------------------------------------------------------------------------
QSharedPointer<ConnectivityComponent> ComponentInstanceLocator::createInstanceData(QSharedPointer<ComponentInstance> instance,
    QSharedPointer<const Component> component, QString const& activeView) const
{
    QSharedPointer<ConnectivityComponent> newInstance(new ConnectivityComponent(instance->getInstanceName()));
    newInstance->setInstanceUuid(instance->getUuid());
    newInstance->setVlnv(instance->getComponentRef()->toString());
    newInstance->setActiveView(activeView);

    addAddressSpaceMemories(newInstance, component);

    addMemoryMapMemories(newInstance, component);

    return newInstance;
}

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::addAddressSpaceMemories()
//-----------------------------------------------------------------------------
void ComponentInstanceLocator::addAddressSpaceMemories(QSharedPointer<ConnectivityComponent> newInstance, 
    QSharedPointer<const Component> component) const
{
    foreach (QSharedPointer<AddressSpace> space, *component->getAddressSpaces())
    {
        QSharedPointer<MemoryItem> spaceItem(new MemoryItem(space->name()));
        spaceItem->setAddress("0");
        spaceItem->setRange(space->getRange());
        spaceItem->setWidth(space->getWidth());

        newInstance->addMemory(spaceItem);

        //addSegmentMemories();
    }
}

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::addMemoryMapMemories()
//-----------------------------------------------------------------------------
void ComponentInstanceLocator::addMemoryMapMemories(QSharedPointer<ConnectivityComponent> instanceData,
    QSharedPointer<const Component> component) const
{
    foreach (QSharedPointer<MemoryMap> map, *component->getMemoryMaps())
    {
        int addressableUnitBits = expressionParser_->parseExpression(map->getAddressUnitBits()).toInt();
        if (addressableUnitBits == 0)
        {
            addressableUnitBits = 8;
        }

        QSharedPointer<MemoryItem> mapItem = createMemoryMapData(map, addressableUnitBits, instanceData);
        instanceData->addMemory(mapItem);

        addMemoryRemapData(map, mapItem, addressableUnitBits, instanceData);
    }
}

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::createMemoryMapData()
//-----------------------------------------------------------------------------
QSharedPointer<MemoryItem> ComponentInstanceLocator::createMemoryMapData(QSharedPointer<MemoryMap> map,
    int addressableUnitBits, QSharedPointer<ConnectivityComponent> containingInstance) const
{
    QString mapIdentifier = containingInstance->getVlnv().replace(':', '.') + "." + 
        containingInstance->getInstanceUuid() + "." + containingInstance->getName() + "." + map->name();

    QSharedPointer<MemoryItem> mapItem(new MemoryItem(map->name()));
    mapItem->setIdentifier(mapIdentifier);

    foreach (QSharedPointer<MemoryBlockBase> block, *map->getMemoryBlocks())
    {
        if (block->getIsPresent().isEmpty() ||
            expressionParser_->parseExpression(block->getIsPresent()).toInt() == 1)
        {
            QSharedPointer<AddressBlock> addressBlock = block.dynamicCast<AddressBlock>();
            QSharedPointer<MemoryItem> blockItem = createMemoryBlock(addressBlock, mapIdentifier, addressableUnitBits);

            mapItem->addChild(blockItem);
        }
    }

    return mapItem;
}

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::createMemoryBlock()
//-----------------------------------------------------------------------------
QSharedPointer<MemoryItem> ComponentInstanceLocator::createMemoryBlock(QSharedPointer<AddressBlock> addressBlock,
    QString const& mapIdentifier, int addressableUnitBits) const
{
    QString blockIdentifier = mapIdentifier + "." + addressBlock->name();
    int baseAddress = expressionParser_->parseExpression(addressBlock->getBaseAddress()).toInt();

    QSharedPointer<MemoryItem> blockItem(new MemoryItem(addressBlock->name()));
    blockItem->setIdentifier(blockIdentifier);
    blockItem->setAddress(QString::number(baseAddress));
    blockItem->setRange(expressionParser_->parseExpression(addressBlock->getRange()));
    blockItem->setWidth(expressionParser_->parseExpression(addressBlock->getWidth()));

    foreach (QSharedPointer<RegisterBase> registerBase, *addressBlock->getRegisterData())
    {
        if (registerBase->getIsPresent().isEmpty() || 
            expressionParser_->parseExpression(registerBase->getIsPresent()).toInt() == 1)
        {
            QSharedPointer<Register> reg = registerBase.dynamicCast<Register>();

            addRegisterData(reg, baseAddress, addressableUnitBits, blockIdentifier, blockItem);
        }
    }

    return blockItem;
}

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::addRegisterData()
//-----------------------------------------------------------------------------
void ComponentInstanceLocator::addRegisterData(QSharedPointer<Register> reg, int baseAddress, 
    int addressableUnitBits, QString const& blockIdentifier, QSharedPointer<MemoryItem> blockItem) const
{
    int registerAddress = baseAddress + expressionParser_->parseExpression(reg->getAddressOffset()).toInt();    
    int registerSize =  expressionParser_->parseExpression(reg->getSize()).toInt();
    int registerDimension = expressionParser_->parseExpression(reg->getDimension()).toInt();

    for (int i = 0; i < registerDimension || (i == 0 && registerDimension == 0); i++)
    {
        QString registerIdentifier = blockIdentifier + "." + reg->name();
        if (!reg->getDimension().isEmpty())
        {
            registerIdentifier.append(QString("[%1]").arg(i));
        }

        QSharedPointer<MemoryItem> regItem(new MemoryItem(reg->name()));
        regItem->setIdentifier(registerIdentifier);
        regItem->setAddress(QString::number(registerAddress));
        regItem->setSize(expressionParser_->parseExpression(reg->getSize()));

        foreach (QSharedPointer<Field> field, *reg->getFields())
        {
            if (field->getIsPresent().isEmpty() || 
                expressionParser_->parseExpression(field->getIsPresent()).toInt() == 1)
            {
                regItem->addChild(createField(field, registerIdentifier, registerAddress, addressableUnitBits));
            }
        }

        registerAddress += registerSize / addressableUnitBits;      

        blockItem->addChild(regItem);
    }
}

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::createField()
//-----------------------------------------------------------------------------
QSharedPointer<MemoryItem> ComponentInstanceLocator::createField(QSharedPointer<Field> field, 
    QString const& registerIdentifier, int regAddress, int addressableUnitBits) const
{
    QString fieldIdentifier = registerIdentifier + "." + field->name();
    int bitOffset = expressionParser_->parseExpression(field->getBitOffset()).toInt();
   
    QSharedPointer<MemoryItem> fieldItem(new MemoryItem(field->name()));
    fieldItem->setIdentifier(fieldIdentifier);
    fieldItem->setWidth(expressionParser_->parseExpression(field->getBitWidth()));
    fieldItem->setAddress(QString::number(regAddress + bitOffset/addressableUnitBits));
    fieldItem->setOffset(QString::number(bitOffset % addressableUnitBits));

    return fieldItem;
}

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::addMemoryRemapData()
//-----------------------------------------------------------------------------
void ComponentInstanceLocator::addMemoryRemapData(QSharedPointer<MemoryMap> map, 
    QSharedPointer<MemoryItem> mapItem, int addressableUnitBits, QSharedPointer<ConnectivityComponent> containingInstance) const
{
    foreach (QSharedPointer<MemoryRemap> remap, *map->getMemoryRemaps())
    {
        QString remapIdentifier =  containingInstance->getVlnv().replace(':', '.') + "." + 
            containingInstance->getInstanceUuid() + "." + containingInstance->getName() + "." + remap->name();

        foreach (QSharedPointer<MemoryBlockBase> block, *remap->getMemoryBlocks())
        {
            if (block->getIsPresent().isEmpty() ||
                expressionParser_->parseExpression(block->getIsPresent()).toInt() == 1)
            {
                QSharedPointer<AddressBlock> addressBlock = block.dynamicCast<AddressBlock>();

                mapItem->addChild(createMemoryBlock(addressBlock, remapIdentifier, addressableUnitBits));
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::addSubInstances()
//-----------------------------------------------------------------------------
void ComponentInstanceLocator::addSubInstances(QSharedPointer<ConnectivityComponent> topInstance, 
    QSharedPointer<const Component> topComponent,
    QSharedPointer<const DesignConfiguration> designConfiguration, QSharedPointer<ConnectivityGraph> graph)
{
    QString viewName = "";
    if (designConfiguration)
    {
        viewName = designConfiguration->getActiveView(topInstance->getName());
    }

    if (viewName.isEmpty() && topComponent->getViews()->count() == 1)
    {
        viewName = topComponent->getViews()->first()->name();
    }

    if (!viewName.isEmpty())
    {
        foreach (QSharedPointer<View> componentView, *topComponent->getViews())
        {
            if (componentView->name() == viewName && componentView->isHierarchical())
            {
                QSharedPointer<const DesignConfiguration> hierarchicalConfiguration =
                    getHierarchicalDesignConfiguration(topComponent, componentView);
                QSharedPointer<const Design> hierarchicalDesign = getHierarchicalDesign(topComponent,
                    componentView, hierarchicalConfiguration);

                analyzeDesign(hierarchicalDesign, hierarchicalConfiguration, topInstance->getName(), graph);
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::getHierarchicalDesignConfiguration()
//-----------------------------------------------------------------------------
QSharedPointer<const DesignConfiguration> ComponentInstanceLocator::getHierarchicalDesignConfiguration(
    QSharedPointer<const Component> component, QSharedPointer<View> hierarchicalView) const
{
    QString configurationReference = hierarchicalView->getDesignConfigurationInstantiationRef();
    if (!configurationReference.isEmpty())
    {
        foreach (QSharedPointer<DesignConfigurationInstantiation> instantiation,
            *component->getDesignConfigurationInstantiations())
        {
            if (instantiation->name() == configurationReference)
            {
                QSharedPointer<ConfigurableVLNVReference> configurationVLNV =
                    instantiation->getDesignConfigurationReference();

                QSharedPointer<const Document> configurationDocument =
                    library_->getModelReadOnly(*configurationVLNV);

                return configurationDocument.dynamicCast<const DesignConfiguration>();
            }
        }
    }

    return QSharedPointer<const DesignConfiguration>();
}

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::getHierarchicalDesign()
//-----------------------------------------------------------------------------
QSharedPointer<const Design> ComponentInstanceLocator::getHierarchicalDesign(QSharedPointer<const Component> component,
    QSharedPointer<View> hierarchicalView, QSharedPointer<const DesignConfiguration> designConfiguration) const
{
    VLNV designVLNV = getHierarchicalDesignVLNV(component, hierarchicalView);

    if (!designVLNV.isValid())
    {
        designVLNV = designConfiguration->getDesignRef();        
    }

    if (designVLNV.isValid())
    {
        return library_->getModelReadOnly(designVLNV).dynamicCast<const Design>();
    }

    return QSharedPointer<Design>();
}

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::getHierarchicalDesignVLNV()
//-----------------------------------------------------------------------------
VLNV ComponentInstanceLocator::getHierarchicalDesignVLNV(QSharedPointer<const Component> component,
    QSharedPointer<View> hierarchicalView) const
{
    QString referencedInstantiation = hierarchicalView->getDesignInstantiationRef();

    if (!referencedInstantiation.isEmpty())
    {
        foreach (QSharedPointer<DesignInstantiation> instantiation, *component->getDesignInstantiations())
        {
            if (instantiation->name() == referencedInstantiation)
            {
                return *instantiation->getDesignReference();
            }
        }
    }

    return VLNV();
}

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::createInterfaceData()
//-----------------------------------------------------------------------------
QSharedPointer<ConnectivityInterface> ComponentInstanceLocator::createInterfaceData(QSharedPointer<BusInterface> busInterface,
    QSharedPointer<ConnectivityComponent> instanceNode)
{
    QSharedPointer<ConnectivityInterface> interfaceNode(new ConnectivityInterface(busInterface->name()));
    interfaceNode->setMode(General::interfaceMode2Str(busInterface->getInterfaceMode()));                   
    interfaceNode->setInstance(instanceNode);

    if (busInterface->getInterfaceMode() == General::MASTER)
    {
        interfaceNode->setBaseAddress(busInterface->getMaster()->getBaseAddress());
    }

    if (busInterface->getInterfaceMode() == General::MIRROREDSLAVE)
    {
        interfaceNode->setRemapAddress(
            busInterface->getMirroredSlave()->getRemapAddresses()->first()->remapAddress_);
    }

    QString memoryReference = busInterface->getAddressSpaceRef();
    if (memoryReference.isEmpty())
    {
        memoryReference = busInterface->getMemoryMapRef();
    }

    if (!memoryReference.isEmpty())
    {
        foreach (QSharedPointer<MemoryItem> memory, instanceNode->getMemories())
        {
            if (memory->getName() == memoryReference)
            {
                interfaceNode->setConnectedMemory(memory);
            }
        }
    }

    return interfaceNode;
}

//-----------------------------------------------------------------------------
// Function: ComponentInstanceLocator::addConnections()
//-----------------------------------------------------------------------------
void ComponentInstanceLocator::addConnections(QSharedPointer<const Design> design,
    QString const& topInstanceName, QSharedPointer<ConnectivityGraph> graph)
{
    foreach (QSharedPointer<Interconnection> interconnection, *design->getInterconnections())
    {
        QSharedPointer<ActiveInterface> start = interconnection->getStartInterface();

        QSharedPointer<ConnectivityInterface> startInterface = graph->getInterface(start->getComponentReference(),
            start->getBusReference());

        if (!startInterface.isNull())
        {
            foreach (QSharedPointer<HierInterface> hierInterface, *interconnection->getHierInterfaces())
            {
                QSharedPointer<ConnectivityInterface> target = graph->getInterface(topInstanceName,  
                    hierInterface->getBusReference());

                 graph->addConnection(interconnection->name(), startInterface, target);
            }

            foreach (QSharedPointer<ActiveInterface> activeInterface, *interconnection->getActiveInterfaces())
            {                
                QSharedPointer<ConnectivityInterface> target = graph->getInterface(activeInterface->getComponentReference(), 
                    activeInterface->getBusReference());

                 graph->addConnection(interconnection->name(), startInterface, target);
            }
        }
    }
}
