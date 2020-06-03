#ifndef MISCSETTINGS_H
#define MISCSETTINGS_H

#include <QColor>

class MiscSettings
{
public:
    MiscSettings();

    // TODO: Maybe not the best place for this stuff...
    QColor backgroundColor;
    bool show_grid = true;
    bool show_selection_outline = true;
    bool use_bloom = true;
    bool use_ssao = true;
    bool renderLightSources = true;
};

#endif // MISCSETTINGS_H
