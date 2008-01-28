/*
* Ultra Defragmenter report sorting engine.
* Copyright (C) 2008 by Dmitri Arkhangelski (dmitriar@gmail.com).
*/

/*
* Interface:
*	1. init_sorting_engine()
*	2. sort_items(criteria), where criteria is 
*		{"fragments" | "size" | "name" | "comment"}
*/

/* Mozilla 1.5.0 and IE 5.0  were tested. */
/* IE 3.0 is not supported. */

/* global variables */
var table;
var whitelist;
var blacklist;
// order = 0 - descending; 1 - ascending
// TODO: set initial values to more useful states
var order = 0;
var fragments_order = 1;
var size_order = 0;
var name_order = 0;
var comment_order = 1;
var msie_browser = false; // true for ms internet explorer

var x; // for debugging purposes

// TODO: get this from html page
var table_head =
"<table border=\"1\" color=\"#FFAA55\" cellspacing=\"0\" width=\"100%\">";

function init_sorting_engine()
{
	table = document.getElementsByTagName("table")[0];
	//alert(window.navigator.appName);
	if(window.navigator.appName == "Microsoft Internet Explorer")
		msie_browser = true;
}

function sort_items(criteria)
{
	var i;
	whitelist = "";
	blacklist = "";
	var a = new Array();
	
	var items = table.getElementsByTagName("tr");
	var header = "<tr>" + items[0].innerHTML + "</tr>\n";

	// convert collection to array
	for(i = 1; i < items.length; i++)
		a[i-1] = items[i];

	// sort items
	if(criteria == 'fragments'){
		a.sort(sort_by_fragments);
		fragments_order = fragments_order ? 0 : 1;
	} else if(criteria == 'name'){
		a.sort(sort_by_name);
		name_order = name_order ? 0 : 1;
	} else if(criteria == 'comment'){
		a.sort(sort_by_comment);
		comment_order = comment_order ? 0 : 1;
	} else {
		return; // invalid criteria
	}
	//alert(x);
	//dump_array_of_items(a);

	// loop through the array of sorted items
	for(i = 0; i < a.length; i++){
		var data = a[i].innerHTML;
		if(a[i].className == "u")
			whitelist += "<tr class=\"u\">" + data + "</tr>\n";
		else
			blacklist += "<tr class=\"f\">" + data + "</tr>\n";
	}

	// replace old contents with new sorted
	if(!msie_browser){
		table.innerHTML = header + whitelist + blacklist;
	} else {
		// On f...ed Internet Explorer table.innerHTML is read only !!!
		// and we need to replace the whole table ...
		document.getElementById("for_msie").innerHTML =
			table_head + header + whitelist + blacklist + "</table>";
		table = document.getElementsByTagName("table")[0];
	}
}

function sort_by_fragments(a,b)
{
	var fa = parseInt(a.getElementsByTagName("td")[0].innerHTML);
	var fb = parseInt(b.getElementsByTagName("td")[0].innerHTML);

	if(fragments_order)
		return (fa - fb);
	else
		return (fb - fa);
}

function sort_by_name(a,b)
{
	var na = a.getElementsByTagName("td")[1].innerHTML.toLowerCase();
	var nb = b.getElementsByTagName("td")[1].innerHTML.toLowerCase();
	var result;
	if(na > nb) result = 1;
	else if(na == nb) result = 0;
	else result = -1;
	
	if(name_order)
		return -result;
	else
		return result;
}

function sort_by_comment(a,b)
{
	var ca = a.getElementsByTagName("td")[2].innerHTML.toLowerCase();
	var cb = b.getElementsByTagName("td")[2].innerHTML.toLowerCase();
	var result;
	if(ca > cb) result = 1;
	else if(ca == cb) result = 0;
	else result = -1;
	
	if(comment_order)
		return -result;
	else
		return result;
}

// for debugging purposes
function dump_table()
{
	alert(table.innerHTML);
}

function dump_array_of_items(a)
{
	var i;
	var result = "";
	
	for(i = 0; i < a.length; i++)
		result += a[i].innerHTML + "\n";
	alert(result);
}
