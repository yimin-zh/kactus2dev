//-----------------------------------------------------------------------------
// File: MemoryDesignerConstants.h
//-----------------------------------------------------------------------------
// Project: Kactus 2
// Author: Mikko Teuho
// Date: 23.09.2016
//
// Description:
// Common declarations for memory designer.
//-----------------------------------------------------------------------------

#include <designEditors/common/diagramgrid.h>

#include <QString>

// Common declarations for memory designer.
namespace MemoryDesignerConstants
{
    //! Starting widths of the memory columns.
    const int SPACECOLUMNWIDTH = 519;
    const int MEMORYMAPCOLUMNWIDTH = 1569;

    //! Interval of one bit in a memory address.
    const qreal RANGEINTERVAL = GridSize * 1.5;

    //! Minimum interval of address space graphics items.
    const qreal SPACEITEMINTERVAL = GridSize * 8;

    //! X-position of the memory map graphics item sub items.
    const qreal MAPSUBITEMPOSITIONX = RANGEINTERVAL * 4.5;

    const QString RESERVED_NAME = QLatin1String("Reserved");

    const QString MEMORYMAP_TYPE = QLatin1String("memoryMap");
    const QString ADDRESSBLOCK_TYPE = QLatin1String("addressBlock");
    const QString REGISTER_TYPE = QLatin1String("register");
    const QString FIELD_TYPE = QLatin1String("field");
    const QString ADDRESSSPACE_TYPE = QLatin1String("addressSpace");
    const QString ADDRESSSEGMENT_TYPE = QLatin1String("segment");

    const QString ADDRESSSPACECOLUMN_NAME = QLatin1String("Address Space");
    const QString MEMORYMAPCOLUMNCOMMON_NAME = QLatin1String("Memory Map");
    const QString MEMORYMAPCOLUM_NAME = QLatin1String("Memory Maps");
    const QString MEMORYMAPOVERLAPCOLUMN_NAME = QLatin1String("Memory Map Overlap");
}
