#pragma once
/*=========================================================================

 medInria

 Copyright (c) INRIA 2013 - 2020. All rights reserved.
 See LICENSE.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

=========================================================================*/

#include <QString>
#include <QSize>

#include <medCoreLegacyExport.h>

namespace med
{
    MEDCORELEGACY_EXPORT QString smartBaseName(const QString & fileName);

    struct GPUInfo
    {
        QString renderer;
        QString version;
        QString vendor;
    };

    MEDCORELEGACY_EXPORT GPUInfo gpuModel();

    static const QSize defaultThumbnailSize = QSize(320,320);

}