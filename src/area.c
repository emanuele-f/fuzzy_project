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

#define _centered_x ((ulong)((FUZZY_AREA_NCOLS) / 2))
#define _centered_y ((ulong)((FUZZY_AREA_NROWS) / 2))

//~ #define DUMP_DEBUG

FuzzyArea FuzzyMeleeMan = {
    {1, 1, 1},
    {1, 0, 1},
    {1, 1, 1}
};

FuzzyArea FuzzyRangedMan = {
    {0, 0, 1, 1, 1, 0, 0},
    {0, 1, 1, 1, 1, 1, 0},
    {1, 1, 0, 0, 0, 1, 1},
    {1, 1, 0, 0, 0, 1, 1},
    {1, 1, 0, 0, 0, 1, 1},
    {0, 1, 1, 1, 1, 1, 0},
    {0, 0, 1, 1, 1, 0, 0},
};

static void _dump_area(FuzzyArea area)
{
    uint i, j;
    
    for(i=0; i<FUZZY_AREA_NROWS; i++) {
        for(j=0; j<FUZZY_AREA_NCOLS; j++)
            printf("%d ", area[i][j]);
        puts("");
    }
}

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
            
    /* position the prototype at the center */
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

#define _pivotted_to_local_x(pivot_x, x) (x - (pivot_x - _centered_x))
#define _pivotted_to_local_y(pivot_y, y) (y - (pivot_y - _centered_y))
#define _local_to_pivotted_x(pivot_x, lx) (lx + pivot_x - _centered_x)
#define _local_to_pivotted_y(pivot_y, ly) (ly + pivot_y - _centered_y)

void fuzzy_area_iter_begin(const FuzzyArea * area, FuzzyAreaIterator * iterator, const FuzzyPoint * pivot, const FuzzyPoint * limit)
{
    iterator->_pivot.x = pivot->x;
    iterator->_pivot.y = pivot->y;
    iterator->_limit.x = _pivotted_to_local_x(pivot->x, limit->x);
    iterator->_limit.y = _pivotted_to_local_y(pivot->y, limit->y);
    
    if (pivot->x > _centered_x) {
        iterator->pos.x = _local_to_pivotted_x(pivot->x, 0);
        iterator->_apos.x = 0;
    } else {
        iterator->_apos.x = _pivotted_to_local_x(pivot->x, _centered_x - pivot->x);
        iterator->pos.x = _centered_x - pivot->x;
    }
        
    if (pivot->y > _centered_y) {
        iterator->_apos.y = 0;
        iterator->pos.y = _local_to_pivotted_x(pivot->y, 0);
    } else {
        iterator->_apos.y = _pivotted_to_local_y(pivot->y, _centered_y - pivot->y);
        iterator->pos.y = _centered_y - pivot->y;
    }
}

FuzzyAreaIterator * fuzzy_area_iter(const FuzzyArea * area, FuzzyAreaIterator * iterator)
{
    ulong locx, locy;
    
    locx = iterator->_apos.x;
    locy = iterator->_apos.y;
    
    if (locy >= FUZZY_AREA_NROWS || locy >= iterator->_limit.y)
        // finished
        return NULL;
    
    iterator->pos.x = _local_to_pivotted_x(iterator->_pivot.x, locx);
    iterator->pos.y = _local_to_pivotted_y(iterator->_pivot.y, locy);
    iterator->value = (*area)[locx][locy];
    
    locx++;
    if (locx >= FUZZY_AREA_NCOLS || locx >= iterator->_limit.x) {
        locx = 0;
        locy++;
    }
    
    iterator->_apos.x = locx;
    iterator->_apos.y = locy;
    
    return iterator;
}

bool fuzzy_area_inside(const FuzzyArea * area, const FuzzyPoint * pivot, const FuzzyPoint * check)
{
    ulong locx, locy;
    
    
    locx = _pivotted_to_local_x(pivot->x, check->x);
    locy = _pivotted_to_local_y(pivot->y, check->y);
    
    return ((*area)[locx][locy]);
}

void fuzzy_areadb_init()
{
    fuzzy_debug("Generating area sets...");
    
    fuzzy_area_prototype(FuzzyMeleeMan, 3, 3);
    fuzzy_area_prototype(FuzzyRangedMan, 7, 7);
    
    fuzzy_debug("Generation done");
}
