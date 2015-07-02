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
#include "server.h"

int main()
{
    char srvkey[FUZZY_SERVERKEY_LEN];

    fuzzy_server_create(FUZZY_DEFAULT_SERVER_PORT, srvkey);
    fuzzy_server_loop(NULL);
    //~ fuzzy_nz_rerror(pthread_join(srv_thread, &retval));
    //~ fuzzy_server_destroy();
    return EXIT_SUCCESS;
}
