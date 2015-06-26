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

typedef struct FuzzyPoint {
    ulong x;
    ulong y;
} FuzzyPoint;

/* keep these odd numbers */
#define FUZZY_AREA_NROWS 15
#define FUZZY_AREA_NCOLS 15

typedef bool FuzzyArea[FUZZY_AREA_NROWS][FUZZY_AREA_NCOLS];

/** Use prototype area as a reference to build a complete area.
 
    \param area prototype, and result container
    \param width of the prototype
    \param height of the prototype
 
    \note width and height must be odd numbers
    \note the prototype area is expanded to FUZZY_AREA_NROWSxFUZZY_AREA_NCOLS
 */
void fuzzy_area_prototype(FuzzyArea * area, uint width, uint height);

/** Checks if given point is inside area descriptor.
 
   \param area descriptor
   \param pivot central area point
   \param check test point
   
   \retval true is inside
   \retval false is outside
 */
void fuzzy_area_inside(const FuzzyArea * area, const FuzzyPoint * pivot, const FuzzyPoint * check);

/** Initializes the iterator for the iteration.
    \param area
    \param pivot central area point
    \param iterator the variable to initialize
 */
void fuzzy_area_iter_begin(const FuzzyArea * area, const FuzzyPoint * pivot, FuzzyPoint ** iterator);

/** Iterates through the area cells
    
    \param area
    \param pivot central area point
    \param iterator a variable, initialized by fuzzy_area_begin_iter, updated
           during each iteration.
 
    \retval the same iterator value
    \retval NULL if iteration is finished
 */
FuzzyPoint * fuzzy_area_iter(const FuzzyArea * area, const FuzzyPoint * pivot, FuzzyPoint ** iterator);

void fuzzy_areadb_init();
