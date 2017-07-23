/** @file
    @brief Linux-specific implementation of getDisplays().

    @date 2016

    @author
    Sensics, Inc.
    <http://sensics.com>

*/

// Copyright 2016 Sensics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// 	http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef INCLUDED_DisplayEnumerator_Linux_h_GUID_CA8EA9D4_36A1_4492_8383_30419BD91FD3
#define INCLUDED_DisplayEnumerator_Linux_h_GUID_CA8EA9D4_36A1_4492_8383_30419BD91FD3

// Internal Includes
#include "DisplayEnumerator.h"
#include "Display.h"

// Library/third-party includes
// - none

// Standard includes
#include <vector>
#include <iostream>
#include <cstring> // strlen

#include <xcb/xcb.h>
#include <xcb/randr.h>

namespace osvr {
namespace display {

std::vector<Display> getDisplays()
{
    std::vector<Display> displays;

    const char *display_str = getenv ( "DISPLAY" );
    xcb_connection_t* XConnection = xcb_connect(display_str, 0);

    xcb_screen_t* XFirstScreen = xcb_setup_roots_iterator(
                               xcb_get_setup(XConnection)).data;
    xcb_window_t XWindowDummy = xcb_generate_id(XConnection);

    xcb_create_window(XConnection, 0, XWindowDummy, XFirstScreen->root,
                      0, 0, 1, 1, 0, 0, 0, 0, 0);

    xcb_flush(XConnection);

    xcb_randr_get_screen_resources_cookie_t screenResCookie = {};
    screenResCookie = xcb_randr_get_screen_resources(XConnection,
                                                     XWindowDummy);

    xcb_randr_get_screen_resources_reply_t* screenResReply = {};
    screenResReply = xcb_randr_get_screen_resources_reply(XConnection,
                     screenResCookie, 0);

    int outputs_num = 0;
    xcb_randr_output_t* firstOutput;

    if(screenResReply)
    {
        outputs_num = xcb_randr_get_screen_resources_outputs_length(screenResReply);

        firstOutput = xcb_randr_get_screen_resources_outputs(screenResReply);
    }
    else
        return {};

    //Array of requests to the X server for CRTC info
    xcb_randr_get_output_info_cookie_t* outputCookie = new
               xcb_randr_get_output_info_cookie_t[outputs_num];
    for(int i = 0; i < outputs_num; i++)
        outputCookie[i] = xcb_randr_get_output_info(XConnection,
                                            *(firstOutput+i), 0);
    //Array of pointers to replies from X server
    xcb_randr_get_output_info_reply_t** outputReply = new
               xcb_randr_get_output_info_reply_t*[outputs_num];
    for(int i = 0; i < outputs_num; i++)
        outputReply[i] = xcb_randr_get_output_info_reply(XConnection,
                                             outputCookie[i], 0);
    //Self-explanatory
    for(int i = 0; i < outputs_num; i++)
    {
        if(outputReply[i] && outputReply[i]->crtc != XCB_NONE)
        {
            Display display;

            DisplayAdapter adapter;
            adapter.description = "TODO";
            display.adapter = std::move(adapter);

            uint8_t* outputname = xcb_randr_get_output_info_name(outputReply[i]);
            int len = xcb_randr_get_output_info_name_length(outputReply[i]);
            std::string name = std::string((char*)outputname, len);
            display.name = name;

            xcb_randr_get_crtc_info_cookie_t ct = xcb_randr_get_crtc_info ( XConnection, outputReply[i]->crtc, XCB_CURRENT_TIME );
            xcb_randr_get_crtc_info_reply_t *crtc_reply = xcb_randr_get_crtc_info_reply ( XConnection, ct, NULL );
            if ( crtc_reply ) {
                DisplaySize display_size;
                display_size.height = crtc_reply->height;
                display_size.width = crtc_reply->width;
                display.size = display_size;

                DisplayPosition display_position;
                display_position.x = crtc_reply->x;
                display_position.y = crtc_reply->y;
                display.position = display_position;

                uint16_t rotation = crtc_reply->rotation;
                if (rotation == 1) {
                    display.rotation = Rotation::Zero;
                } else if (rotation == 4) {
                    display.rotation = Rotation::Ninety;
                } else if (rotation == 2) {
                    display.rotation = Rotation::TwoSeventy;
                } else if (rotation == 8) {
                    display.rotation = Rotation::OneEighty;
                }
                //std::cout << rotation << std::endl;

                display.attachedToDesktop = true;

                //TODO: refresh rate from the monitor the dummy window is on??
                xcb_randr_get_screen_info_reply_t *r =
                xcb_randr_get_screen_info_reply (XConnection,
                                                 xcb_randr_get_screen_info (XConnection, XWindowDummy), NULL);
                display.verticalRefreshRate = r->rate;

            }


            /* // TODO: edid values
            xcb_generic_error_t *error;
            xcb_intern_atom_cookie_t edid_cookie = xcb_intern_atom (XConnection, 1, strlen("EDID"), "EDID");
            xcb_intern_atom_reply_t *edid_reply = xcb_intern_atom_reply (XConnection, edid_cookie, &error);
            if (error != NULL || edid_reply == NULL) {
                int ec = error ? error->error_code : -1;
                fprintf (stderr, "Intern Atom returned error %d\n", ec);
            }

            xcb_randr_query_output_property_cookie_t prop_cookie = xcb_randr_query_output_property (XConnection, *firstOutput, edid_reply->atom);
            xcb_randr_query_output_property_reply_t *prop_reply = xcb_randr_query_output_property_reply(XConnection, prop_cookie, &error);
            int vallen = xcb_randr_query_output_property_valid_values_length (prop_reply);
            int32_t *values = xcb_randr_query_output_property_valid_values (prop_reply);
            std::cout << "vals" << std::endl;
            for (int i = 0; i < vallen; i++)
                std::cout << values[i] << std::endl;
            */

            //display.edidVendorId =
            //display.edidProductId =


            displays.emplace_back(std::move(display));
        }
    }
    xcb_disconnect(XConnection);

    return displays;
}

DesktopOrientation getDesktopOrientation(const Display& display)
{
    const auto rotation = display.rotation;

    if (Rotation::Zero == rotation) {
        return (DesktopOrientation::Landscape);
    } else if (Rotation::Ninety == rotation) {
        return (DesktopOrientation::PortraitFlipped); //TODO: Maybe need to switch Portrait and PortraitFlipped
    } else if (Rotation::OneEighty == rotation) {
        return (DesktopOrientation::LandscapeFlipped);
    } else if (Rotation::TwoSeventy == rotation) {
        return (DesktopOrientation::Portrait);
    } else {
        std::cerr << "Invalid rotation value: " << static_cast<int>(rotation) << ".";
        return DesktopOrientation::Landscape;
    }
}

} // end namespace display
} // end namespace osvr

#endif // INCLUDED_DisplayEnumerator_Linux_h_GUID_CA8EA9D4_36A1_4492_8383_30419BD91FD3

