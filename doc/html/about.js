/*
* Scripting engine for the 'About' page of the Ultra Defragmenter.
* Copyright (c) 2008 by Dmitri Arkhangelski.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

var msie_browser = false;
var opera_browser = false;
var win;

function init(){
	// The following code must be compatible with IE 3.0
	if(window.navigator.appName == "Microsoft Internet Explorer"){
		msie_browser = true;

		var ver = window.navigator.appVersion;
		var verHi = parseInt(ver);
		var agent = window.navigator.userAgent.toLowerCase();
		var ie4_browser = ((ver == 4) && (agent.indexOf("msie 5.0") == -1));
		if(ver < 4 || ie4_browser){ // the browser is too old
			window.navigate("about_simple.html");
			window.status = "Your browser is too old!";
			return;
		}
		// The next code could be incompatible with IE 3.0
		var cross = document.getElementById("cross");
		cross.style.cursor = "hand";
		cross.style.width = cross.style.height = "11pt";
	}

	if(window.navigator.appName == "Opera")
		opera_browser = true;

	if(msie_browser || opera_browser){
		var notes = document.getElementById("notes_list");
		notes.style.top = "12pt";
	}

	win = document.getElementById("window");
	onResize(null);

	// for debugging
	window.onresize = onResize;
}

function onResize(e){
	// I don't know why -40
	//win.style.display = "none";
	var left = (getClientWidth() - parseInt(win.style.width)) / 2 - 40;
	if(left < 0) left = 0;
	var top = (getClientHeight() - parseInt(win.style.height)) / 2 - 40;
	if(top < 0) top = 0;
	win.style.left = left + "px";
	win.style.top = top + "px";
	//win.style.display = "block";
}

function close_win(){
	window.close();
}

// Helper functions from http://www.tigir.com/javascript.htm
//IE5+, Mozilla 1.0+, Opera 7+

function getClientWidth(){
  return document.compatMode=='CSS1Compat' && !window.opera?document.documentElement.clientWidth:document.body.clientWidth;
}

function getClientHeight(){
  return document.compatMode=='CSS1Compat' && !window.opera?document.documentElement.clientHeight:document.body.clientHeight;
}
