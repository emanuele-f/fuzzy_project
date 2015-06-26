/*
 * Emanuele Faranda         black.silver@hotmail.it
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "fuzzy.h"
#include "area.h"

#define _central_x (FUZZY_AREA_NCOLS / 2 + 1)
#define _central_y (FUZZY_AREA_NROWS / 2 + 1)

//~ #define DUMP_DEBUG

FuzzyArea FuzzyRangedMan = {
    {0, 0, 1, 1, 1, 0, 0},
    {0, 1, 1, 1, 1, 1, 0},
    {1, 1, 0, 0, 0, 1, 1},
    {1, 1, 0, 0, 0, 1, 1},
    {1, 1, 0, 0, 0, 1, 1},
    {0, 1, 1, 1, 1, 1, 0},
    {0, 0, 1, 1, 1, 0, 0},
};

#ifdef DUMP_DEBUG
static void _dump_area(FuzzyArea area)
{
    uint i, j;
    
    for(i=0; i<FUZZY_AREA_NROWS; i++) {
        for(j=0; j<FUZZY_AREA_NCOLS; j++)
            printf("%d ", area[i][j]);
        puts("");
    }
}
#endif

void fuzzy_area_prototype(FuzzyArea area, uint width, uint height)
{
    FuzzyArea support;
    uint i, j, xoff, yoff;

#ifdef DUMP_DEBUG
    _dump_area(area);
#endif
    
    if (width % 2 == 0)
        fuzzy_critical(fuzzy_sformat("Prototype width is not odd: %d", width));
    if (width > FUZZY_AREA_NCOLS)
        fuzzy_critical(fuzzy_sformat("Prototype width is too big: %d", width));
        
    if (height % 2 == 0)
        fuzzy_critical(fuzzy_sformat("Prototype height is not odd: %d", height));
    if (height > FUZZY_AREA_NROWS)
        fuzzy_critical(fuzzy_sformat("Prototype height is too big: %d", height));
    
    /* zero support area */
    for(i=0; i<FUZZY_AREA_NROWS; i++)
        for(j=0; j<FUZZY_AREA_NCOLS; j++)
            support[i][j] = 0;
            
    /* position the prototype to the center */
    xoff = (FUZZY_AREA_NROWS-height)/2;
    yoff = (FUZZY_AREA_NCOLS-width)/2;
    for (i=0; i<height; i++)
        for(j=0; j<width; j++)
            support[i+xoff][j+yoff] = area[i][j];
            
    /* copy back to origin */
    for(i=0; i<FUZZY_AREA_NROWS; i++)
        for(j=0; j<FUZZY_AREA_NCOLS; j++)
            area[i][j] = support[i][j];
    
#ifdef DUMP_DEBUG
    puts("");
    _dump_area(area);
#endif
}

void fuzzy_areadb_init()
{
    fuzzy_debug("Generating area sets...");
    
    fuzzy_area_prototype(FuzzyRangedMan, 7, 7);
    
    fuzzy_debug("Generation done");
}
