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
 * Lightweight macroes to handle linked lists
 *
 */

#ifndef __FUZZY_LIST_H
#define __FUZZY_LIST_H

/* Declare a list item */
#define fuzzy_list_link(type) type * _next

#define fuzzy_list_null(item)\
    item->_next = NULL

/* Append to the end of list */
#define fuzzy_list_append(type, head, item)\
do{\
    if (head == NULL) {\
        head = item;\
    } else {\
        type * _p = head;\
        while(_p->_next)\
            _p = _p->_next;\
        _p->_next = item;\
    }\
}while(0)

/* Prepend to the beginning of the list */
#define fuzzy_list_prepend(head, item)\
do{\
    item->_next = head;\
    head = item;\
}while(0)

#define fuzzy_list_remove(type, head, ptr)\
do{\
    type * _p = NULL;\
    type * _c = head;\
    while(_c) {\
        if (_c == ptr) {\
            if (! _p)\
                head = _c->_next;\
            else\
                _p->_next = _c->_next;\
        }\
        _p = _c;\
        _c = _c->_next;\
    }\
}while(0)

#define fuzzy_list_next(iterator)\
    (iterator = iterator->_next)

#define fuzzy_list_next_ptr(item)\
    (item->_next)

/* Find an item by attribute equal comparison */
#define fuzzy_list_findbyattr(type, head, attr, value, found)\
do{\
    type * _p = head;\
    while(_p) {\
        if (_p->attr == value)\
            break;\
        fuzzy_list_next(_p);\
    }\
    found = _p;\
}while(0)

#define fuzzy_list_map(type, head, callback)\
do{\
    type * _p = head;\
    type * _d = NULL;\
    while(_p) {\
        _d = _p;\
        _p = _p->_next;\
        callback(_d);\
    }\
}while(0)

#endif
