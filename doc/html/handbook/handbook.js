/* handbook.js - contains ancillary functions for the UltraDefrag handbook */
/* License: Public Domain */

function show_section(id,image)
{
	var section;
	var img;
	
	if(document.all){
		section = document.all[id];
		img = document.all[image];
	} else {
		section = document.getElementById(id);
		img = document.getElementById(image);
	}

	if(section.style.display == 'none'){
		section.style.display = 'block';
		img.src = "open.gif"
	} else {
		section.style.display = 'none';
		img.src = "closed.gif"
	}
}
