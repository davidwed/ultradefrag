var fileLoadingImage = "/lightbox_assets/images/loading.gif";		
var fileBottomNavCloseImage = "/lightbox_assets/images/closelabel.gif";
var overlayOpacity = 0.8;
var animate = true;
var resizeSpeed = 7;
var borderSize = 10;
var imageArray = new Array;
var activeImage;
if(animate == true){
	overlayDuration = 0.2;
	if(resizeSpeed > 10){ resizeSpeed = 10;}
	if(resizeSpeed < 1){ resizeSpeed = 1;}
	resizeDuration = (11 - resizeSpeed) * 0.15;
} else { 
	overlayDuration = 0;
	resizeDuration = 0;
}
Object.extend(Element, {
	getWidth: function(element) {
	   	element = $(element);
	   	return element.offsetWidth; 
	},
	setWidth: function(element,w) {
	   	element = $(element);
    	element.style.width = w +"px";
	},
	setHeight: function(element,h) {
   		element = $(element);
    	element.style.height = h +"px";
	},
	setTop: function(element,t) {
	   	element = $(element);
    	element.style.top = t +"px";
	},
	setLeft: function(element,l) {
	   	element = $(element);
    	element.style.left = l +"px";
	},
	setSrc: function(element,src) {
    	element = $(element);
    	element.src = src; 
	},
	setHref: function(element,href) {
    	element = $(element);
    	element.href = href; 
	},
	setInnerHTML: function(element,content) {
		element = $(element);
		element.innerHTML = content;
	}
});
Array.prototype.removeDuplicates = function () {
    for(i = 0; i < this.length; i++){
        for(j = this.length-1; j>i; j--){        
            if(this[i][0] == this[j][0]){
                this.splice(j,1);
            }
        }
    }
}
Array.prototype.empty = function () {
	for(i = 0; i <= this.length; i++){
		this.shift();
	}
}
var Lightbox = Class.create();
Lightbox.prototype = {
	initialize: function() {	
		
		this.updateImageList();
		var objBody = document.getElementsByTagName("body").item(0);
		var objOverlay = document.createElement("div");
		objOverlay.setAttribute('id','overlay');
		objOverlay.style.display = 'none';
		objOverlay.onclick = function() { myLightbox.end(); }
		objBody.appendChild(objOverlay);
		var objLightbox = document.createElement("div");
		objLightbox.setAttribute('id','lightbox');
		objLightbox.style.display = 'none';
		objLightbox.onclick = function(e) {
			if (!e) var e = window.event;
			var clickObj = Event.element(e).id;
			if ( clickObj == 'lightbox') {
				myLightbox.end();
			}
		};
		objBody.appendChild(objLightbox);	
		var objOuterImageContainer = document.createElement("div");
		objOuterImageContainer.setAttribute('id','outerImageContainer');
		objLightbox.appendChild(objOuterImageContainer);
		if(animate){
			Element.setWidth('outerImageContainer', 250);
			Element.setHeight('outerImageContainer', 250);			
		} else {
			Element.setWidth('outerImageContainer', 1);
			Element.setHeight('outerImageContainer', 1);			
		}
		var objImageContainer = document.createElement("div");
		objImageContainer.setAttribute('id','imageContainer');
		objOuterImageContainer.appendChild(objImageContainer);
		var objLightboxImage = document.createElement("img");
		objLightboxImage.setAttribute('id','lightboxImage');
		objImageContainer.appendChild(objLightboxImage);
		var objHoverNav = document.createElement("div");
		objHoverNav.setAttribute('id','hoverNav');
		objImageContainer.appendChild(objHoverNav);
		var objPrevLink = document.createElement("a");
		objPrevLink.setAttribute('id','prevLink');
		objPrevLink.setAttribute('href','#');
		objHoverNav.appendChild(objPrevLink);
		var objNextLink = document.createElement("a");
		objNextLink.setAttribute('id','nextLink');
		objNextLink.setAttribute('href','#');
		objHoverNav.appendChild(objNextLink);
		var objLoading = document.createElement("div");
		objLoading.setAttribute('id','loading');
		objImageContainer.appendChild(objLoading);
		var objLoadingLink = document.createElement("a");
		objLoadingLink.setAttribute('id','loadingLink');
		objLoadingLink.setAttribute('href','#');
		objLoadingLink.onclick = function() { myLightbox.end(); return false; }
		objLoading.appendChild(objLoadingLink);
		var objLoadingImage = document.createElement("img");
		objLoadingImage.setAttribute('src', fileLoadingImage);
		objLoadingLink.appendChild(objLoadingImage);
		var objImageDataContainer = document.createElement("div");
		objImageDataContainer.setAttribute('id','imageDataContainer');
		objLightbox.appendChild(objImageDataContainer);
		var objImageData = document.createElement("div");
		objImageData.setAttribute('id','imageData');
		objImageDataContainer.appendChild(objImageData);
		var objImageDetails = document.createElement("div");
		objImageDetails.setAttribute('id','imageDetails');
		objImageData.appendChild(objImageDetails);
		var objCaption = document.createElement("span");
		objCaption.setAttribute('id','caption');
		objImageDetails.appendChild(objCaption);
		var objNumberDisplay = document.createElement("span");
		objNumberDisplay.setAttribute('id','numberDisplay');
		objImageDetails.appendChild(objNumberDisplay);
		var objBottomNav = document.createElement("div");
		objBottomNav.setAttribute('id','bottomNav');
		objImageData.appendChild(objBottomNav);
		var objBottomNavCloseLink = document.createElement("a");
		objBottomNavCloseLink.setAttribute('id','bottomNavClose');
		objBottomNavCloseLink.setAttribute('href','#');
		objBottomNavCloseLink.onclick = function() { myLightbox.end(); return false; }
		objBottomNav.appendChild(objBottomNavCloseLink);
		var objBottomNavCloseImage = document.createElement("img");
		objBottomNavCloseImage.setAttribute('src', fileBottomNavCloseImage);
		objBottomNavCloseLink.appendChild(objBottomNavCloseImage);
	},
	updateImageList: function() {	
		if (!document.getElementsByTagName){ return; }
		var anchors = document.getElementsByTagName('a');
		var areas = document.getElementsByTagName('area');
		for (var i=0; i<anchors.length; i++){
			var anchor = anchors[i];
			var relAttribute = String(anchor.getAttribute('rel'));
			if (anchor.getAttribute('href') && (relAttribute.toLowerCase().match('lightbox'))){
				anchor.onclick = function () {myLightbox.start(this); return false;}
			}
		}
		for (var i=0; i< areas.length; i++){
			var area = areas[i];
			
			var relAttribute = String(area.getAttribute('rel'));
			if (area.getAttribute('href') && (relAttribute.toLowerCase().match('lightbox'))){
				area.onclick = function () {myLightbox.start(this); return false;}
			}
		}
	},
	start: function(imageLink) {	
		hideSelectBoxes();
		hideFlash();
		var arrayPageSize = getPageSize();
		Element.setWidth('overlay', arrayPageSize[0]);
		Element.setHeight('overlay', arrayPageSize[1]);
		new Effect.Appear('overlay', { duration: overlayDuration, from: 0.0, to: overlayOpacity });
		imageArray = [];
		imageNum = 0;		
		if (!document.getElementsByTagName){ return; }
		var anchors = document.getElementsByTagName( imageLink.tagName);
		if((imageLink.getAttribute('rel') == 'lightbox')){
			imageArray.push(new Array(imageLink.getAttribute('href'), imageLink.getAttribute('title')));			
		} else {

			for (var i=0; i<anchors.length; i++){
				var anchor = anchors[i];
				if (anchor.getAttribute('href') && (anchor.getAttribute('rel') == imageLink.getAttribute('rel'))){
					imageArray.push(new Array(anchor.getAttribute('href'), anchor.getAttribute('title')));
				}
			}
			imageArray.removeDuplicates();
			while(imageArray[imageNum][0] != imageLink.getAttribute('href')) { imageNum++;}
		}
		var arrayPageScroll = getPageScroll();
		var lightboxTop = arrayPageScroll[1] + (arrayPageSize[3] / 10);
		var lightboxLeft = arrayPageScroll[0];
		Element.setTop('lightbox', lightboxTop);
		Element.setLeft('lightbox', lightboxLeft);
		Element.show('lightbox');
		this.changeImage(imageNum);
	},
	changeImage: function(imageNum) {	
		activeImage = imageNum;

		if(animate){ Element.show('loading');}
		Element.hide('lightboxImage');
		Element.hide('hoverNav');
		Element.hide('prevLink');
		Element.hide('nextLink');
		Element.hide('imageDataContainer');
		Element.hide('numberDisplay');		
		imgPreloader = new Image();
		imgPreloader.onload=function(){
			Element.setSrc('lightboxImage', imageArray[activeImage][0]);
			myLightbox.resizeImageContainer(imgPreloader.width, imgPreloader.height);
			
			imgPreloader.onload=function(){};
		}
		imgPreloader.src = imageArray[activeImage][0];
	},
	resizeImageContainer: function( imgWidth, imgHeight) {
		this.widthCurrent = Element.getWidth('outerImageContainer');
		this.heightCurrent = Element.getHeight('outerImageContainer');
		var widthNew = (imgWidth  + (borderSize * 2));
		var heightNew = (imgHeight  + (borderSize * 2));
		this.xScale = ( widthNew / this.widthCurrent) * 100;
		this.yScale = ( heightNew / this.heightCurrent) * 100;
		wDiff = this.widthCurrent - widthNew;
		hDiff = this.heightCurrent - heightNew;
		if(!( hDiff == 0)){ new Effect.Scale('outerImageContainer', this.yScale, {scaleX: false, duration: resizeDuration, queue: 'front'}); }
		if(!( wDiff == 0)){ new Effect.Scale('outerImageContainer', this.xScale, {scaleY: false, delay: resizeDuration, duration: resizeDuration}); }
		if((hDiff == 0) && (wDiff == 0)){
			if (navigator.appVersion.indexOf("MSIE")!=-1){ pause(250); } else { pause(100);} 
		}
		Element.setHeight('prevLink', imgHeight);
		Element.setHeight('nextLink', imgHeight);
		Element.setWidth( 'imageDataContainer', widthNew);

		this.showImage();
	},
	showImage: function(){
		Element.hide('loading');
		new Effect.Appear('lightboxImage', { duration: resizeDuration, queue: 'end', afterFinish: function(){	myLightbox.updateDetails(); } });
		this.preloadNeighborImages();
	},
	updateDetails: function() {
		if(imageArray[activeImage][1]){
			Element.show('caption');
			Element.setInnerHTML( 'caption', imageArray[activeImage][1]);
		}
		if(imageArray.length > 1){
			Element.show('numberDisplay');
			Element.setInnerHTML( 'numberDisplay', "Image " + eval(activeImage + 1) + " of " + imageArray.length);
		}

		new Effect.Parallel(
			[ new Effect.SlideDown( 'imageDataContainer', { sync: true, duration: resizeDuration, from: 0.0, to: 1.0 }), 
			  new Effect.Appear('imageDataContainer', { sync: true, duration: resizeDuration }) ], 
			{ duration: resizeDuration, afterFinish: function() {
				var arrayPageSize = getPageSize();
				Element.setHeight('overlay', arrayPageSize[1]);
				myLightbox.updateNav();
				}
			} 
		);
	},
	updateNav: function() {

		Element.show('hoverNav');				
		if(activeImage != 0){
			Element.show('prevLink');
			document.getElementById('prevLink').onclick = function() {
				myLightbox.changeImage(activeImage - 1); return false;
			}
		}
		if(activeImage != (imageArray.length - 1)){
			Element.show('nextLink');
			document.getElementById('nextLink').onclick = function() {
				myLightbox.changeImage(activeImage + 1); return false;
			}
		}
		
		this.enableKeyboardNav();
	},
	enableKeyboardNav: function() {
		document.onkeydown = this.keyboardAction; 
	},
	disableKeyboardNav: function() {
		document.onkeydown = '';
	},

	keyboardAction: function(e) {
		if (e == null) {
			keycode = event.keyCode;
			escapeKey = 27;
		} else {
			keycode = e.keyCode;
			escapeKey = e.DOM_VK_ESCAPE;
		}

		key = String.fromCharCode(keycode).toLowerCase();
		
		if((key == 'x') || (key == 'o') || (key == 'c') || (keycode == escapeKey)){
			myLightbox.end();
		} else if((key == 'p') || (keycode == 37)){	
			if(activeImage != 0){
				myLightbox.disableKeyboardNav();
				myLightbox.changeImage(activeImage - 1);
			}
		} else if((key == 'n') || (keycode == 39)){
			if(activeImage != (imageArray.length - 1)){
				myLightbox.disableKeyboardNav();
				myLightbox.changeImage(activeImage + 1);
			}
		}

	},
	preloadNeighborImages: function(){

		if((imageArray.length - 1) > activeImage){
			preloadNextImage = new Image();
			preloadNextImage.src = imageArray[activeImage + 1][0];
		}
		if(activeImage > 0){
			preloadPrevImage = new Image();
			preloadPrevImage.src = imageArray[activeImage - 1][0];
		}
	
	},
	end: function() {
		this.disableKeyboardNav();
		Element.hide('lightbox');
		new Effect.Fade('overlay', { duration: overlayDuration});
		showSelectBoxes();
		showFlash();
	}
}
function getPageScroll(){
	var xScroll, yScroll;
	if (self.pageYOffset) {
		yScroll = self.pageYOffset;
		xScroll = self.pageXOffset;
	} else if (document.documentElement && document.documentElement.scrollTop){
		yScroll = document.documentElement.scrollTop;
		xScroll = document.documentElement.scrollLeft;
	} else if (document.body) {
		yScroll = document.body.scrollTop;
		xScroll = document.body.scrollLeft;	
	}
	arrayPageScroll = new Array(xScroll,yScroll) 
	return arrayPageScroll;
}
function getPageSize(){
	var xScroll, yScroll;
	if (window.innerHeight && window.scrollMaxY) {	
		xScroll = window.innerWidth + window.scrollMaxX;
		yScroll = window.innerHeight + window.scrollMaxY;
	} else if (document.body.scrollHeight > document.body.offsetHeight){
		xScroll = document.body.scrollWidth;
		yScroll = document.body.scrollHeight;
	} else {
		xScroll = document.body.offsetWidth;
		yScroll = document.body.offsetHeight;
	}
	var windowWidth, windowHeight;

	if (self.innerHeight) {
		if(document.documentElement.clientWidth){
			windowWidth = document.documentElement.clientWidth; 
		} else {
			windowWidth = self.innerWidth;
		}
		windowHeight = self.innerHeight;
	} else if (document.documentElement && document.documentElement.clientHeight) {
		windowWidth = document.documentElement.clientWidth;
		windowHeight = document.documentElement.clientHeight;
	} else if (document.body) {
		windowWidth = document.body.clientWidth;
		windowHeight = document.body.clientHeight;
	}
	if(yScroll < windowHeight){
		pageHeight = windowHeight;
	} else { 
		pageHeight = yScroll;
	}
	if(xScroll < windowWidth){	
		pageWidth = xScroll;		
	} else {
		pageWidth = windowWidth;
	}

	arrayPageSize = new Array(pageWidth,pageHeight,windowWidth,windowHeight) 
	return arrayPageSize;
}
function getKey(e){
	if (e == null) {
		keycode = event.keyCode;
	} else {
		keycode = e.which;
	}
	key = String.fromCharCode(keycode).toLowerCase();
	
	if(key == 'x'){
	}
}
function showSelectBoxes(){
	var selects = document.getElementsByTagName("select");
	for (i = 0; i != selects.length; i++) {
		selects[i].style.visibility = "visible";
	}
}

function hideSelectBoxes(){
	var selects = document.getElementsByTagName("select");
	for (i = 0; i != selects.length; i++) {
		selects[i].style.visibility = "hidden";
	}
}
function showFlash(){
	var flashObjects = document.getElementsByTagName("object");
	for (i = 0; i < flashObjects.length; i++) {
		flashObjects[i].style.visibility = "visible";
	}

	var flashEmbeds = document.getElementsByTagName("embed");
	for (i = 0; i < flashEmbeds.length; i++) {
		flashEmbeds[i].style.visibility = "visible";
	}
}

function hideFlash(){
	var flashObjects = document.getElementsByTagName("object");
	for (i = 0; i < flashObjects.length; i++) {
		flashObjects[i].style.visibility = "hidden";
	}
	var flashEmbeds = document.getElementsByTagName("embed");
	for (i = 0; i < flashEmbeds.length; i++) {
		flashEmbeds[i].style.visibility = "hidden";
	}

}
function pause(ms){
	var date = new Date();
	curDate = null;
	do{var curDate = new Date();}
	while( curDate - date < ms);
}
function initLightbox() { myLightbox = new Lightbox(); }