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


#include <xcb/xcb.h>
#include <xcb/randr.h>

namespace osvr {
namespace display {

std::vector<Display> getDisplays()
{
    std::vector<Display> displays;


    //Open connection to X server
    xcb_connection_t* XConnection = xcb_connect(0, 0);

    //Get the first X screen
    xcb_screen_t* XFirstScreen = xcb_setup_roots_iterator(
                               xcb_get_setup(XConnection)).data;
    //Generate ID for the X window
    xcb_window_t XWindowDummy = xcb_generate_id(XConnection);

    //Create dummy X window
    xcb_create_window(XConnection, 0, XWindowDummy, XFirstScreen->root,
                      0, 0, 1, 1, 0, 0, 0, 0, 0);

    //Flush pending requests to the X server
    xcb_flush(XConnection);

    //Send a request for screen resources to the X server
    xcb_randr_get_screen_resources_cookie_t screenResCookie = {};
    screenResCookie = xcb_randr_get_screen_resources(XConnection,
                                                     XWindowDummy);

    //Receive reply from X server
    xcb_randr_get_screen_resources_reply_t* screenResReply = {};
    screenResReply = xcb_randr_get_screen_resources_reply(XConnection,
                     screenResCookie, 0);

    int outputs_num = 0;
    xcb_randr_crtc_t* firstCRTC;

    //Get a pointer to the first CRTC and number of CRTCs
    //It is crucial to notice that you are in fact getting
    //an array with firstCRTC being the first element of
    //this array and crtcs_length - length of this array
    if(screenResReply)
    {
        outputs_num = xcb_randr_get_screen_resources_outputs_length(screenResReply);

        firstCRTC = xcb_randr_get_screen_resources_outputs(screenResReply);
    }
    else
        return {};

    //Array of requests to the X server for CRTC info
    xcb_randr_get_output_info_cookie_t* outputCookie = new
               xcb_randr_get_output_info_cookie_t[outputs_num];
    for(int i = 0; i < outputs_num; i++)
        outputCookie[i] = xcb_randr_get_output_info(XConnection,
                                            *(firstCRTC+i), 0);
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

            /*
            printf("CRTC[%i] INFO:\n", i);
            printf("x-off\t: %i\n", outputReply[i]->x);
            printf("y-off\t: %i\n", outputReply[i]->y);
            printf("width\t: %i\n", outputReply[i]->width);
            printf("height\t: %i\n\n", outputReply[i]->height);
            */

            DisplayAdapter adapter;
            adapter.description = "TODO";
            display.adapter = std::move(adapter);

            uint8_t * outputname = xcb_randr_get_output_info_name(outputReply[i]);
            int len = xcb_randr_get_output_info_name_length(outputReply[i]);
            std::string name = std::string((char*)outputname, len);
            display.name = name;

            xcb_randr_crtc_t crtc = outputReply[i]->crtc;

            displays.emplace_back(std::move(display));
        }
    }

    xcb_disconnect(XConnection);

    Display display;
    /*
    display.adapter = getDisplayAdapter(display_id);
    display.name = getDisplayName(display_id);
    display.size = getDisplaySize(display_id);
    display.position = getDisplayPosition(display_id);
    display.rotation = getDisplayRotation(display_id);
    display.verticalRefreshRate = getDisplayRefreshRate(display_id);
    display.attachedToDesktop = getDisplayAttachedToDesktop(display_id);
    display.edidVendorId = getDisplayEDIDVendorID(display_id);
    display.edidProductId = getDisplayEDIDProductID(display_id);
    */
    return displays;
}

DesktopOrientation getDesktopOrientation(const Display& display)
{
	// TODO
	return DesktopOrientation::Landscape;
}

} // end namespace display
} // end namespace osvr

#endif // INCLUDED_DisplayEnumerator_Linux_h_GUID_CA8EA9D4_36A1_4492_8383_30419BD91FD3

