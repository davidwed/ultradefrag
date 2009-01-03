var Spry;if(!Spry)Spry={};Spry.forwards=1;Spry.backwards=2;Spry.linearTransition=1;Spry.sinusoidalTransition=2;if(!Spry.Effect)Spry.Effect={};Spry.Effect.Registry=function()
{this.elements=new Array();_AnimatedElement=function(element)
{this.element=element;this.currentEffect=-1;this.effectArray=new Array();};this.AnimatedElement=_AnimatedElement;};Spry.Effect.Registry.prototype.getRegisteredEffect=function(element,effect)
{var eleIdx=this.getIndexOfElement(element);if(eleIdx==-1)
{this.elements[this.elements.length]=new this.AnimatedElement(element);eleIdx=this.elements.length-1;}
var foundEffectArrayIdx=-1;for(var i=0;i<this.elements[eleIdx].effectArray.length;i++)
{if(this.elements[eleIdx].effectArray[i])
{if(this.effectsAreTheSame(this.elements[eleIdx].effectArray[i],effect))
{foundEffectArrayIdx=i;if(this.elements[eleIdx].effectArray[i].isRunning==true){this.elements[eleIdx].effectArray[i].cancel();}
this.elements[eleIdx].currentEffect=i;if(this.elements[eleIdx].effectArray[i].options&&(this.elements[eleIdx].effectArray[i].options.toggle!=null)){if(this.elements[eleIdx].effectArray[i].options.toggle==true)
this.elements[eleIdx].effectArray[i].doToggle();}else{this.elements[eleIdx].effectArray[i]=effect;}
break;}}}
if(foundEffectArrayIdx==-1)
{var currEffectIdx=this.elements[eleIdx].effectArray.length;this.elements[eleIdx].effectArray[currEffectIdx]=effect;this.elements[eleIdx].currentEffect=currEffectIdx;}
var idx=this.elements[eleIdx].currentEffect;return this.elements[eleIdx].effectArray[idx];}
Spry.Effect.Registry.prototype.getIndexOfElement=function(element)
{var registryIndex=-1;for(var i=0;i<this.elements.length;i++)
{if(this.elements[i]){if(this.elements[i].element==element)
registryIndex=i;}}
return registryIndex;}
Spry.Effect.Registry.prototype.effectsAreTheSame=function(effectA,effectB)
{if(effectA.name!=effectB.name)
return false;if(effectA.effectsArray)
{if(!effectB.effectsArray||effectA.effectsArray.length!=effectB.effectsArray.length)
return false;for(var i=0;i<effectA.effectsArray.length;i++)
{if(!Spry.Effect.Utils.optionsAreIdentical(effectA.effectsArray[i].effect.options,effectB.effectsArray[i].effect.options))
return false;}}
else
{if(effectB.effectsArray||!Spry.Effect.Utils.optionsAreIdentical(effectA.options,effectB.options))
return false;}
return true;}
var SpryRegistry=new Spry.Effect.Registry;if(!Spry.Effect.Utils)Spry.Effect.Utils={};Spry.Effect.Utils.showError=function(msg)
{alert('Spry.Effect ERR: '+msg);}
Spry.Effect.Utils.Position=function()
{this.x=0;this.y=0;this.units="px";}
Spry.Effect.Utils.Rectangle=function()
{this.width=0;this.height=0;this.units="px";}
Spry.Effect.Utils.PositionedRectangle=function()
{this.position=new Spry.Effect.Utils.Position;this.rectangle=new Spry.Effect.Utils.Rectangle;}
Spry.Effect.Utils.intToHex=function(integerNum)
{var result=integerNum.toString(16);if(result.length==1)
result="0"+result;return result;}
Spry.Effect.Utils.hexToInt=function(hexStr)
{return parseInt(hexStr,16);}
Spry.Effect.Utils.rgb=function(redInt,greenInt,blueInt)
{var redHex=Spry.Effect.Utils.intToHex(redInt);var greenHex=Spry.Effect.Utils.intToHex(greenInt);var blueHex=Spry.Effect.Utils.intToHex(blueInt);compositeColorHex=redHex.concat(greenHex,blueHex);compositeColorHex='#'+compositeColorHex;return compositeColorHex;}
Spry.Effect.Utils.camelize=function(stringToCamelize)
{var oStringList=stringToCamelize.split('-');var isFirstEntry=true;var camelizedString='';for(var i=0;i<oStringList.length;i++)
{if(oStringList[i].length>0)
{if(isFirstEntry)
{camelizedString=oStringList[i];isFirstEntry=false;}
else
{var s=oStringList[i];camelizedString+=s.charAt(0).toUpperCase()+s.substring(1);}}}
return camelizedString;}
Spry.Effect.Utils.isPercentValue=function(value)
{var result=false;try
{if(value.lastIndexOf("%")>0)
result=true;}
catch(e){}
return result;}
Spry.Effect.Utils.getPercentValue=function(value)
{var result=0;try
{result=Number(value.substring(0,value.lastIndexOf("%")));}
catch(e){Spry.Effect.Utils.showError('Spry.Effect.Utils.getPercentValue: '+e);}
return result;}
Spry.Effect.Utils.getPixelValue=function(value)
{var result=0;try
{result=Number(value.substring(0,value.lastIndexOf("px")));}
catch(e){}
return result;}
Spry.Effect.Utils.getFirstChildElement=function(node)
{if(node)
{var childCurr=node.firstChild;while(childCurr)
{if(childCurr.nodeType==1)
return childCurr;childCurr=childCurr.nextSibling;}}
return null;};Spry.Effect.Utils.fetchChildImages=function(startEltIn,targetImagesOut)
{if(!startEltIn||startEltIn.nodeType!=1||!targetImagesOut)
return;if(startEltIn.hasChildNodes())
{var childImages=startEltIn.getElementsByTagName('img')
var imageCnt=childImages.length;for(var i=0;i<imageCnt;i++)
{var imgCurr=childImages[i];var dimensionsCurr=Spry.Effect.getDimensions(imgCurr);targetImagesOut.push([imgCurr,dimensionsCurr.width,dimensionsCurr.height]);}}}
Spry.Effect.Utils.optionsAreIdentical=function(optionsA,optionsB)
{if(optionsA==null&&optionsB==null)
return true;if(optionsA!=null&&optionsB!=null)
{var objectCountA=0;var objectCountB=0;for(var propA in optionsA)objectCountA++;for(var propB in optionsB)objectCountB++;if(objectCountA!=objectCountB)
return false;for(var prop in optionsA)
{if(optionsA[prop]===undefined)
{if(optionsB[prop]!==undefined)
return false;}
else if((optionsB[prop]===undefined)||(optionsA[prop]!=optionsB[prop]))
{return false;}}
return true;}
return false;}
Spry.Effect.getElement=function(ele)
{var element=null;if(ele&&typeof ele=="string")
element=document.getElementById(ele);else
element=ele;if(element==null)Spry.Effect.Utils.showError('Element "'+ele+'" not found.');return element;}
Spry.Effect.getStyleProp=function(element,prop)
{var value;try
{value=element.style[Spry.Effect.Utils.camelize(prop)];if(!value)
{if(document.defaultView&&document.defaultView.getComputedStyle){var css=document.defaultView.getComputedStyle(element,null);value=css?css.getPropertyValue(prop):null;}else
if(element.currentStyle){value=element.currentStyle[Spry.Effect.Utils.camelize(prop)];}}}
catch(e){Spry.Effect.Utils.showError('Spry.Effect.getStyleProp: '+e);}
return value=='auto'?null:value;};Spry.Effect.getStylePropRegardlessOfDisplayState=function(element,prop,displayElement)
{var refElement=displayElement?displayElement:element;var displayOrig=Spry.Effect.getStyleProp(refElement,'display');var visibilityOrig=Spry.Effect.getStyleProp(refElement,'visibility');if(displayOrig=='none')
{Spry.Effect.setStyleProp(refElement,'visibility','hidden');Spry.Effect.setStyleProp(refElement,'display','block');if(window.opera)
refElement.focus();}
var styleProp=Spry.Effect.getStyleProp(element,prop);if(displayOrig=='none')
{Spry.Effect.setStyleProp(refElement,'display','none');Spry.Effect.setStyleProp(refElement,'visibility',visibilityOrig);}
return styleProp;};Spry.Effect.setStyleProp=function(element,prop,value)
{try
{element.style[Spry.Effect.Utils.camelize(prop)]=value;}
catch(e){Spry.Effect.Utils.showError('Spry.Effect.setStyleProp: '+e);}
return null;};Spry.Effect.makePositioned=function(element)
{var pos=Spry.Effect.getStyleProp(element,'position');if(!pos||pos=='static'){element.style.position='relative';if(window.opera){element.style.top=0;element.style.left=0;}}}
Spry.Effect.isInvisible=function(element)
{var propDisplay=Spry.Effect.getStyleProp(element,'display');if(propDisplay&&propDisplay.toLowerCase()=='none')
return true;var propVisible=Spry.Effect.getStyleProp(element,'visibility');if(propVisible&&propVisible.toLowerCase()=='hidden')
return true;return false;}
Spry.Effect.enforceVisible=function(element)
{var propDisplay=Spry.Effect.getStyleProp(element,'display');if(propDisplay&&propDisplay.toLowerCase()=='none')
Spry.Effect.setStyleProp(element,'display','block');var propVisible=Spry.Effect.getStyleProp(element,'visibility');if(propVisible&&propVisible.toLowerCase()=='hidden')
Spry.Effect.setStyleProp(element,'visibility','visible');}
Spry.Effect.makeClipping=function(element)
{var overflow=Spry.Effect.getStyleProp(element,'overflow');if(overflow!='hidden'&&overflow!='scroll')
{var heightCache=0;var needsCache=/MSIE 7.0/.test(navigator.userAgent)&&/Windows NT/.test(navigator.userAgent);if(needsCache)
heightCache=Spry.Effect.getDimensionsRegardlessOfDisplayState(element).height;Spry.Effect.setStyleProp(element,'overflow','hidden');if(needsCache)
Spry.Effect.setStyleProp(element,'height',heightCache+'px');}}
Spry.Effect.cleanWhitespace=function(element)
{var childCountInit=element.childNodes.length;for(var i=childCountInit-1;i>=0;i--){var node=element.childNodes[i];if(node.nodeType==3&&!/\S/.test(node.nodeValue))
{try
{element.removeChild(node);}
catch(e){Spry.Effect.Utils.showError('Spry.Effect.cleanWhitespace: '+e);}}}}
Spry.Effect.getComputedStyle=function(element)
{var computedStyle=/MSIE/.test(navigator.userAgent)?element.currentStyle:document.defaultView.getComputedStyle(element,null);return computedStyle;}
Spry.Effect.getDimensions=function(element)
{var dimensions=new Spry.Effect.Utils.Rectangle;var computedStyle=null;if(element.style.width&&/px/i.test(element.style.width))
{dimensions.width=parseInt(element.style.width);}
else
{computedStyle=Spry.Effect.getComputedStyle(element);var tryComputedStyle=computedStyle&&computedStyle.width&&/px/i.test(computedStyle.width);if(tryComputedStyle)
dimensions.width=parseInt(computedStyle.width);if(!tryComputedStyle||dimensions.width==0)
dimensions.width=element.offsetWidth;}
if(element.style.height&&/px/i.test(element.style.height))
{dimensions.height=parseInt(element.style.height);}
else
{if(!computedStyle)
computedStyle=Spry.Effect.getComputedStyle(element);var tryComputedStyle=computedStyle&&computedStyle.height&&/px/i.test(computedStyle.height);if(tryComputedStyle)
dimensions.height=parseInt(computedStyle.height);if(!tryComputedStyle||dimensions.height==0)
dimensions.height=element.offsetHeight;}
return dimensions;}
Spry.Effect.getDimensionsRegardlessOfDisplayState=function(element,displayElement)
{var refElement=displayElement?displayElement:element;var displayOrig=Spry.Effect.getStyleProp(refElement,'display');var visibilityOrig=Spry.Effect.getStyleProp(refElement,'visibility');if(displayOrig=='none')
{Spry.Effect.setStyleProp(refElement,'visibility','hidden');Spry.Effect.setStyleProp(refElement,'display','block');if(window.opera)
refElement.focus();}
var dimensions=Spry.Effect.getDimensions(element);if(displayOrig=='none')
{Spry.Effect.setStyleProp(refElement,'display','none');Spry.Effect.setStyleProp(refElement,'visibility',visibilityOrig);}
return dimensions;}
Spry.Effect.getOpacity=function(element)
{var o=Spry.Effect.getStyleProp(element,"opacity");if(o==undefined||o==null)
o=1.0;return o;}
Spry.Effect.getColor=function(element)
{var c=Spry.Effect.getStyleProp(ele,"background-color");return c;}
Spry.Effect.getPosition=function(element)
{var position=new Spry.Effect.Utils.Position;var computedStyle=null;if(element.style.left&&/px/i.test(element.style.left))
{position.x=parseInt(element.style.left);}
else
{computedStyle=Spry.Effect.getComputedStyle(element);var tryComputedStyle=computedStyle&&computedStyle.left&&/px/i.test(computedStyle.left);if(tryComputedStyle)
position.x=parseInt(computedStyle.left);if(!tryComputedStyle||position.x==0)
position.x=element.offsetLeft;}
if(element.style.top&&/px/i.test(element.style.top))
{position.y=parseInt(element.style.top);}
else
{if(!computedStyle)
computedStyle=Spry.Effect.getComputedStyle(element);var tryComputedStyle=computedStyle&&computedStyle.top&&/px/i.test(computedStyle.top);if(tryComputedStyle)
position.y=parseInt(computedStyle.top);if(!tryComputedStyle||position.y==0)
position.y=element.offsetTop;}
return position;}
Spry.Effect.getOffsetPosition=Spry.Effect.getPosition;Spry.Effect.Animator=function(options)
{this.name='Animator';this.element=null;this.timer=null;this.direction=Spry.forwards;this.startMilliseconds=0;this.repeat='none';this.isRunning=false;this.options={duration:500,toggle:false,transition:Spry.linearTransition,interval:33};this.setOptions(options);};Spry.Effect.Animator.prototype.setOptions=function(options)
{if(!options)
return;for(var prop in options)
this.options[prop]=options[prop];};Spry.Effect.Animator.prototype.start=function(withoutTimer)
{if(arguments.length==0)
withoutTimer=false;var self=this;if(this.options.setup)
{try
{this.options.setup(this.element,this);}
catch(e){Spry.Effect.Utils.showError('Spry.Effect.Animator.prototype.start: setup callback: '+e);}}
this.prepareStart();var currDate=new Date();this.startMilliseconds=currDate.getTime();if(withoutTimer==false){this.timer=setInterval(function(){self.drawEffect();},this.options.interval);}
this.isRunning=true;};Spry.Effect.Animator.prototype.stop=function()
{if(this.timer){clearInterval(this.timer);this.timer=null;}
this.startMilliseconds=0;if(this.options.finish)
{try
{this.options.finish(this.element,this);}
catch(e){Spry.Effect.Utils.showError('Spry.Effect.Animator.prototype.stop: finish callback: '+e);}}
this.isRunning=false;};Spry.Effect.Animator.prototype.cancel=function()
{if(this.timer){clearInterval(this.timer);this.timer=null;}
this.isRunning=false;}
Spry.Effect.Animator.prototype.drawEffect=function()
{var isRunning=true;var position=this.getElapsedMilliseconds()/this.options.duration;if(this.getElapsedMilliseconds()>this.options.duration){position=1.0;}else{if(this.options.transition==Spry.sinusoidalTransition)
{position=(-Math.cos(position*Math.PI)/2)+0.5;}
else if(this.options.transition==Spry.linearTransition)
{}
else
{Spry.Effect.Utils.showError('unknown transition');}}
this.animate(position);if(this.getElapsedMilliseconds()>this.options.duration){this.stop();isRunning=false;}
return isRunning;};Spry.Effect.Animator.prototype.getElapsedMilliseconds=function()
{if(this.startMilliseconds>0){var currDate=new Date();return(currDate.getTime()-this.startMilliseconds);}else{return 0;}};Spry.Effect.Animator.prototype.doToggle=function()
{if(this.options.toggle==true){if(this.direction==Spry.forwards){this.direction=Spry.backwards;}else if(this.direction==Spry.backwards){this.direction=Spry.forwards;}}}
Spry.Effect.Animator.prototype.prepareStart=function(){};Spry.Effect.Animator.prototype.animate=function(position){};Spry.Effect.Move=function(element,fromPos,toPos,options)
{this.dynamicFromPos=false;if(arguments.length==3)
{options=toPos;toPos=fromPos;fromPos=Spry.Effect.getPosition(element);this.dynamicFromPos=true;}
Spry.Effect.Animator.call(this,options);this.name='Move';this.element=Spry.Effect.getElement(element);if(fromPos.units!=toPos.units)
Spry.Effect.Utils.showError('Spry.Effect.Move: Conflicting units ('+fromPos.units+', '+toPos.units+')');this.units=fromPos.units;this.startX=fromPos.x;this.stopX=toPos.x;this.startY=fromPos.y;this.stopY=toPos.y;this.rangeMoveX=this.startX-this.stopX;this.rangeMoveY=this.startY-this.stopY;};Spry.Effect.Move.prototype=new Spry.Effect.Animator();Spry.Effect.Move.prototype.constructor=Spry.Effect.Move;Spry.Effect.Move.prototype.animate=function(position)
{var left=0;var top=0;if(this.direction==Spry.forwards){left=this.startX-(this.rangeMoveX*position);top=this.startY-(this.rangeMoveY*position);}else if(this.direction==Spry.backwards){left=this.rangeMoveX*position+this.stopX;top=this.rangeMoveY*position+this.stopY;}
this.element.style.left=left+this.units;this.element.style.top=top+this.units;};Spry.Effect.Move.prototype.prepareStart=function()
{if(this.dynamicFromPos==true)
{var fromPos=Spry.Effect.getPosition(this.element);this.startX=fromPos.x;this.startY=fromPos.y;this.rangeMoveX=this.startX-this.stopX;this.rangeMoveY=this.startY-this.stopY;}};Spry.Effect.MoveSlide=function(element,fromPos,toPos,horizontal,options)
{this.dynamicFromPos=false;if(arguments.length==4)
{options=horizontal;horizontal=toPos;toPos=fromPos;fromPos=Spry.Effect.getPosition(element);this.dynamicFromPos=true;}
Spry.Effect.Animator.call(this,options);this.name='MoveSlide';this.element=Spry.Effect.getElement(element);this.horizontal=horizontal;this.firstChildElement=Spry.Effect.Utils.getFirstChildElement(element);this.overflow=Spry.Effect.getStyleProp(this.element,'overflow');this.originalChildRect=Spry.Effect.getDimensionsRegardlessOfDisplayState(this.firstChildElement,this.element);if(fromPos.units!=toPos.units)
Spry.Effect.Utils.showError('Spry.Effect.MoveSlide: Conflicting units ('+fromPos.units+', '+toPos.units+')');this.units=fromPos.units;var originalRect=Spry.Effect.getDimensionsRegardlessOfDisplayState(element);this.startHeight=originalRect.height;this.startX=Number(fromPos.x);this.stopX=Number(toPos.x);this.startY=Number(fromPos.y);this.stopY=Number(toPos.y);this.rangeMoveX=this.startX-this.stopX;this.rangeMoveY=this.startY-this.stopY;this.enforceVisible=Spry.Effect.isInvisible(this.element);};Spry.Effect.MoveSlide.prototype=new Spry.Effect.Animator();Spry.Effect.MoveSlide.prototype.constructor=Spry.Effect.MoveSlide;Spry.Effect.MoveSlide.prototype.animate=function(position)
{if(this.horizontal)
{var xStart=(this.direction==Spry.forwards)?this.startX:this.stopX;var xStop=(this.direction==Spry.forwards)?this.stopX:this.startX;var eltWidth=xStart+position*(xStop-xStart);if(eltWidth<0)eltWidth=0;if(this.overflow!='scroll'||eltWidth>this.originalChildRect.width)
this.firstChildElement.style.left=eltWidth-this.originalChildRect.width+this.units;this.element.style.width=eltWidth+this.units;}
else
{var yStart=(this.direction==Spry.forwards)?this.startY:this.stopY;var yStop=(this.direction==Spry.forwards)?this.stopY:this.startY;var eltHeight=yStart+position*(yStop-yStart);if(eltHeight<0)eltHeight=0;if(this.overflow!='scroll'||eltHeight>this.originalChildRect.height)
this.firstChildElement.style.top=eltHeight-this.originalChildRect.height+this.units;this.element.style.height=eltHeight+this.units;}
if(this.enforceVisible)
{Spry.Effect.enforceVisible(this.element);this.enforceVisible=false;}};Spry.Effect.MoveSlide.prototype.prepareStart=function()
{if(this.dynamicFromPos==true)
{var fromPos=Spry.Effect.getPosition(this.element);this.startX=fromPos.x;this.startY=fromPos.y;this.rangeMoveX=this.startX-this.stopX;this.rangeMoveY=this.startY-this.stopY;}};Spry.Effect.Size=function(element,fromRect,toRect,options)
{this.dynamicFromRect=false;if(arguments.length==3)
{options=toRect;toRect=fromRect;fromRect=Spry.Effect.getDimensionsRegardlessOfDisplayState(element);this.dynamicFromRect=true;}
Spry.Effect.Animator.call(this,options);this.name='Size';this.element=Spry.Effect.getElement(element);if(fromRect.units!=toRect.units)
Spry.Effect.Utils.showError('Spry.Effect.Size: Conflicting units ('+fromRect.units+', '+toRect.units+')');this.units=fromRect.units;var originalRect=Spry.Effect.getDimensionsRegardlessOfDisplayState(element);this.originalWidth=originalRect.width;this.startWidth=fromRect.width;this.startHeight=fromRect.height;this.stopWidth=toRect.width;this.stopHeight=toRect.height;this.childImages=new Array();if(this.options.scaleContent)
Spry.Effect.Utils.fetchChildImages(element,this.childImages);this.fontFactor=1.0;if(this.element.style&&this.element.style.fontSize)
{if(/em\s*$/.test(this.element.style.fontSize))
this.fontFactor=parseFloat(this.element.style.fontSize);}
if(Spry.Effect.Utils.isPercentValue(this.startWidth))
{var startWidthPercent=Spry.Effect.Utils.getPercentValue(this.startWidth);this.startWidth=originalRect.width*(startWidthPercent/100);}
if(Spry.Effect.Utils.isPercentValue(this.startHeight))
{var startHeightPercent=Spry.Effect.Utils.getPercentValue(this.startHeight);this.startHeight=originalRect.height*(startHeightPercent/100);}
if(Spry.Effect.Utils.isPercentValue(this.stopWidth))
{var stopWidthPercent=Spry.Effect.Utils.getPercentValue(this.stopWidth);var originalRect=Spry.Effect.getDimensionsRegardlessOfDisplayState(element);this.stopWidth=originalRect.width*(stopWidthPercent/100);}
if(Spry.Effect.Utils.isPercentValue(this.stopHeight))
{var stopHeightPercent=Spry.Effect.Utils.getPercentValue(this.stopHeight);var originalRect=Spry.Effect.getDimensionsRegardlessOfDisplayState(element);this.stopHeight=originalRect.height*(stopHeightPercent/100);}
this.widthRange=this.startWidth-this.stopWidth;this.heightRange=this.startHeight-this.stopHeight;this.enforceVisible=Spry.Effect.isInvisible(this.element);};Spry.Effect.Size.prototype=new Spry.Effect.Animator();Spry.Effect.Size.prototype.constructor=Spry.Effect.Size;Spry.Effect.Size.prototype.animate=function(position)
{var width=0;var height=0;var fontSize=0;if(this.direction==Spry.forwards){width=this.startWidth-(this.widthRange*position);height=this.startHeight-(this.heightRange*position);fontSize=this.fontFactor*(this.startWidth+position*(this.stopWidth-this.startWidth))/this.originalWidth;}else if(this.direction==Spry.backwards){width=this.widthRange*position+this.stopWidth;height=this.heightRange*position+this.stopHeight;fontSize=this.fontFactor*(this.stopWidth+position*(this.startWidth-this.stopWidth))/this.originalWidth;}
if(this.options.scaleContent==true)
this.element.style.fontSize=fontSize+'em';this.element.style.width=width+this.units;this.element.style.height=height+this.units;if(this.options.scaleContent)
{var propFactor=(this.direction==Spry.forwards)?(this.startWidth+position*(this.stopWidth-this.startWidth))/this.originalWidth:(this.stopWidth+position*(this.startWidth-this.stopWidth))/this.originalWidth;for(var i=0;i<this.childImages.length;i++)
{this.childImages[i][0].style.width=propFactor*this.childImages[i][1]+this.units;this.childImages[i][0].style.height=propFactor*this.childImages[i][2]+this.units;}}
if(this.enforceVisible)
{Spry.Effect.enforceVisible(this.element);this.enforceVisible=false;}};Spry.Effect.Size.prototype.prepareStart=function()
{if(this.dynamicFromRect==true)
{var fromRect=Spry.Effect.getDimensions(element);this.startWidth=fromRect.width;this.startHeight=fromRect.height;this.widthRange=this.startWidth-this.stopWidth;this.heightRange=this.startHeight-this.stopHeight;}};Spry.Effect.Opacity=function(element,startOpacity,stopOpacity,options)
{this.dynamicStartOpacity=false;if(arguments.length==3)
{options=stopOpacity;stopOpacity=startOpacity;startOpacity=Spry.Effect.getOpacity(element);this.dynamicStartOpacity=true;}
Spry.Effect.Animator.call(this,options);this.name='Opacity';this.element=Spry.Effect.getElement(element);if(/MSIE/.test(navigator.userAgent)&&(!this.element.hasLayout))
Spry.Effect.setStyleProp(this.element,'zoom','1');this.startOpacity=startOpacity;this.stopOpacity=stopOpacity;this.opacityRange=this.startOpacity-this.stopOpacity;this.enforceVisible=Spry.Effect.isInvisible(this.element);};Spry.Effect.Opacity.prototype=new Spry.Effect.Animator();Spry.Effect.Opacity.prototype.constructor=Spry.Effect.Opacity;Spry.Effect.Opacity.prototype.animate=function(position)
{var opacity=0;if(this.direction==Spry.forwards){opacity=this.startOpacity-(this.opacityRange*position);}else if(this.direction==Spry.backwards){opacity=this.opacityRange*position+this.stopOpacity;}
this.element.style.opacity=opacity;this.element.style.filter="alpha(opacity="+Math.floor(opacity*100)+")";if(this.enforceVisible)
{Spry.Effect.enforceVisible(this.element);this.enforceVisible=false;}};Spry.Effect.Size.prototype.prepareStart=function()
{if(this.dynamicStartOpacity==true)
{this.startOpacity=Spry.Effect.getOpacity(element);this.opacityRange=this.startOpacity-this.stopOpacity;}};Spry.Effect.Color=function(element,startColor,stopColor,options)
{this.dynamicStartColor=false;if(arguments.length==3)
{options=stopColor;stopColor=startColor;startColor=Spry.Effect.getColor(element);this.dynamicStartColor=true;}
Spry.Effect.Animator.call(this,options);this.name='Color';this.element=Spry.Effect.getElement(element);this.startColor=startColor;this.stopColor=stopColor;this.startRedColor=Spry.Effect.Utils.hexToInt(startColor.substr(1,2));this.startGreenColor=Spry.Effect.Utils.hexToInt(startColor.substr(3,2));this.startBlueColor=Spry.Effect.Utils.hexToInt(startColor.substr(5,2));this.stopRedColor=Spry.Effect.Utils.hexToInt(stopColor.substr(1,2));this.stopGreenColor=Spry.Effect.Utils.hexToInt(stopColor.substr(3,2));this.stopBlueColor=Spry.Effect.Utils.hexToInt(stopColor.substr(5,2));this.redColorRange=this.startRedColor-this.stopRedColor;this.greenColorRange=this.startGreenColor-this.stopGreenColor;this.blueColorRange=this.startBlueColor-this.stopBlueColor;};Spry.Effect.Color.prototype=new Spry.Effect.Animator();Spry.Effect.Color.prototype.constructor=Spry.Effect.Color;Spry.Effect.Color.prototype.animate=function(position)
{var redColor=0;var greenColor=0;var blueColor=0;if(this.direction==Spry.forwards){redColor=parseInt(this.startRedColor-(this.redColorRange*position));greenColor=parseInt(this.startGreenColor-(this.greenColorRange*position));blueColor=parseInt(this.startBlueColor-(this.blueColorRange*position));}else if(this.direction==Spry.backwards){redColor=parseInt(this.redColorRange*position)+this.stopRedColor;greenColor=parseInt(this.greenColorRange*position)+this.stopGreenColor;blueColor=parseInt(this.blueColorRange*position)+this.stopBlueColor;}
this.element.style.backgroundColor=Spry.Effect.Utils.rgb(redColor,greenColor,blueColor);};Spry.Effect.Size.prototype.prepareStart=function()
{if(this.dynamicStartColor==true)
{this.startColor=Spry.Effect.getColor(element);this.startRedColor=Spry.Effect.Utils.hexToInt(startColor.substr(1,2));this.startGreenColor=Spry.Effect.Utils.hexToInt(startColor.substr(3,2));this.startBlueColor=Spry.Effect.Utils.hexToInt(startColor.substr(5,2));this.redColorRange=this.startRedColor-this.stopRedColor;this.greenColorRange=this.startGreenColor-this.stopGreenColor;this.blueColorRange=this.startBlueColor-this.stopBlueColor;}};Spry.Effect.Cluster=function(options)
{Spry.Effect.Animator.call(this,options);this.name='Cluster';this.effectsArray=new Array();this.currIdx=-1;_ClusteredEffect=function(effect,kind)
{this.effect=effect;this.kind=kind;this.isRunning=false;};this.ClusteredEffect=_ClusteredEffect;};Spry.Effect.Cluster.prototype=new Spry.Effect.Animator();Spry.Effect.Cluster.prototype.constructor=Spry.Effect.Cluster;Spry.Effect.Cluster.prototype.drawEffect=function()
{var isRunning=true;var allEffectsDidRun=false;if(this.currIdx==-1)
this.initNextEffectsRunning();var baseEffectIsStillRunning=false;var evalNextEffectsRunning=false
for(var i=0;i<this.effectsArray.length;i++)
{if(this.effectsArray[i].isRunning==true)
{baseEffectIsStillRunning=this.effectsArray[i].effect.drawEffect();if(baseEffectIsStillRunning==false&&i==this.currIdx)
{this.effectsArray[i].isRunning=false;evalNextEffectsRunning=true;}}}
if(evalNextEffectsRunning==true)
{allEffectsDidRun=this.initNextEffectsRunning();}
if(allEffectsDidRun==true){this.stop();isRunning=false;for(var i=0;i<this.effectsArray.length;i++)
{this.effectsArray[i].isRunning=false;}
this.currIdx=-1;}
return isRunning;};Spry.Effect.Cluster.prototype.initNextEffectsRunning=function()
{var allEffectsDidRun=false;this.currIdx++;if(this.currIdx>(this.effectsArray.length-1))
{allEffectsDidRun=true;}
else
{for(var i=this.currIdx;i<this.effectsArray.length;i++)
{if((i>this.currIdx)&&this.effectsArray[i].kind=="queue")
break;this.effectsArray[i].effect.start(true);this.effectsArray[i].isRunning=true;this.currIdx=i;};}
return allEffectsDidRun;};Spry.Effect.Cluster.prototype.doToggle=function()
{if(this.options.toggle==true){if(this.direction==Spry.forwards){this.direction=Spry.backwards;}else if(this.direction==Spry.backwards){this.direction=Spry.forwards;}}
for(var i=0;i<this.effectsArray.length;i++)
{if(this.effectsArray[i].effect.options&&(this.effectsArray[i].effect.options.toggle!=null)){if(this.effectsArray[i].effect.options.toggle==true)
{this.effectsArray[i].effect.doToggle();}}}};Spry.Effect.Cluster.prototype.cancel=function()
{for(var i=0;i<this.effectsArray.length;i++)
{this.effectsArray[i].effect.cancel();}
if(this.timer){clearInterval(this.timer);this.timer=null;}
this.isRunning=false;};Spry.Effect.Cluster.prototype.addNextEffect=function(effect)
{this.effectsArray[this.effectsArray.length]=new this.ClusteredEffect(effect,"queue");if(this.effectsArray.length==1){this.element=effect.element;}};Spry.Effect.Cluster.prototype.addParallelEffect=function(effect)
{this.effectsArray[this.effectsArray.length]=new this.ClusteredEffect(effect,"parallel");if(this.effectsArray.length==1){this.element=effect.element;}};Spry.Effect.AppearFade=function(element,options)
{var element=Spry.Effect.getElement(element);var durationInMilliseconds=1000;var fromOpacity=0.0;var toOpacity=100.0;var doToggle=false;var kindOfTransition=Spry.sinusoidalTransition;var setupCallback=null;var finishCallback=null;if(options)
{if(options.duration!=null)durationInMilliseconds=options.duration;if(options.from!=null)fromOpacity=options.from;if(options.to!=null)toOpacity=options.to;if(options.toggle!=null)doToggle=options.toggle;if(options.transition!=null)kindOfTransition=options.transition;if(options.setup!=null)setupCallback=options.setup;if(options.finish!=null)finishCallback=options.finish;}
options={duration:durationInMilliseconds,toggle:doToggle,transition:kindOfTransition,setup:setupCallback,finish:finishCallback,from:fromOpacity,to:toOpacity};fromOpacity=fromOpacity/100.0;toOpacity=toOpacity/100.0;var appearFadeEffect=new Spry.Effect.Opacity(element,fromOpacity,toOpacity,options);appearFadeEffect.name='AppearFade';var registeredEffect=SpryRegistry.getRegisteredEffect(element,appearFadeEffect);registeredEffect.start();return registeredEffect;};Spry.Effect.Blind=function(element,options)
{var element=Spry.Effect.getElement(element);Spry.Effect.makeClipping(element);var durationInMilliseconds=1000;var doToggle=false;var kindOfTransition=Spry.sinusoidalTransition;var doScaleContent=false;var setupCallback=null;var finishCallback=null;var originalRect=Spry.Effect.getDimensionsRegardlessOfDisplayState(element);var fromHeightPx=originalRect.height;var toHeightPx=0;var optionFrom=options?options.from:originalRect.height;var optionTo=options?options.to:0;if(options)
{if(options.duration!=null)durationInMilliseconds=options.duration;if(options.from!=null)
{if(Spry.Effect.Utils.isPercentValue(options.from))
fromHeightPx=Spry.Effect.Utils.getPercentValue(options.from)*originalRect.height/100;else
fromHeightPx=Spry.Effect.Utils.getPixelValue(options.from);}
if(options.to!=null)
{if(Spry.Effect.Utils.isPercentValue(options.to))
toHeightPx=Spry.Effect.Utils.getPercentValue(options.to)*originalRect.height/100;else
toHeightPx=Spry.Effect.Utils.getPixelValue(options.to);}
if(options.toggle!=null)doToggle=options.toggle;if(options.transition!=null)kindOfTransition=options.transition;if(options.setup!=null)setupCallback=options.setup;if(options.finish!=null)finishCallback=options.finish;}
var fromRect=new Spry.Effect.Utils.Rectangle;fromRect.width=originalRect.width;fromRect.height=fromHeightPx;var toRect=new Spry.Effect.Utils.Rectangle;toRect.width=originalRect.width;toRect.height=toHeightPx;options={duration:durationInMilliseconds,toggle:doToggle,transition:kindOfTransition,scaleContent:doScaleContent,setup:setupCallback,finish:finishCallback,from:optionFrom,to:optionTo};var blindEffect=new Spry.Effect.Size(element,fromRect,toRect,options);blindEffect.name='Blind';var registeredEffect=SpryRegistry.getRegisteredEffect(element,blindEffect);registeredEffect.start();return registeredEffect;};function setupHighlight(element,effect)
{Spry.Effect.setStyleProp(element,'background-image','none');};function finishHighlight(element,effect)
{Spry.Effect.setStyleProp(element,'background-image',effect.options.restoreBackgroundImage);if(effect.direction==Spry.forwards)
Spry.Effect.setStyleProp(element,'background-color',effect.options.restoreColor);};Spry.Effect.Highlight=function(element,options)
{var durationInMilliseconds=1000;var toColor="#ffffff";var doToggle=false;var kindOfTransition=Spry.sinusoidalTransition;var setupCallback=setupHighlight;var finishCallback=finishHighlight;var element=Spry.Effect.getElement(element);var fromColor=Spry.Effect.getStyleProp(element,"background-color");var restoreColor=fromColor;if(fromColor=="transparent")fromColor="#ffff99";var optionFrom=options?options.from:'#ffff00';var optionTo=options?options.to:'#0000ff';if(options)
{if(options.duration!=null)durationInMilliseconds=options.duration;if(options.from!=null)fromColor=options.from;if(options.to!=null)toColor=options.to;if(options.restoreColor)restoreColor=options.restoreColor;if(options.toggle!=null)doToggle=options.toggle;if(options.transition!=null)kindOfTransition=options.transition;if(options.setup!=null)setupCallback=options.setup;if(options.finish!=null)finishCallback=options.finish;}
var restoreBackgroundImage=Spry.Effect.getStyleProp(element,'background-image');options={duration:durationInMilliseconds,toggle:doToggle,transition:kindOfTransition,setup:setupCallback,finish:finishCallback,restoreColor:restoreColor,restoreBackgroundImage:restoreBackgroundImage,from:optionFrom,to:optionTo};var highlightEffect=new Spry.Effect.Color(element,fromColor,toColor,options);highlightEffect.name='Highlight';var registeredEffect=SpryRegistry.getRegisteredEffect(element,highlightEffect);registeredEffect.start();return registeredEffect;};Spry.Effect.Slide=function(element,options)
{var element=Spry.Effect.getElement(element);var durationInMilliseconds=2000;var doToggle=false;var kindOfTransition=Spry.sinusoidalTransition;var slideHorizontally=false;var setupCallback=null;var finishCallback=null;var firstChildElt=Spry.Effect.Utils.getFirstChildElement(element);if(/MSIE 7.0/.test(navigator.userAgent)&&/Windows NT/.test(navigator.userAgent))
Spry.Effect.makePositioned(element);Spry.Effect.makeClipping(element);if(/MSIE 6.0/.test(navigator.userAgent)&&/Windows NT/.test(navigator.userAgent))
{var pos=Spry.Effect.getStyleProp(element,'position');if(pos&&(pos=='static'||pos=='fixed'))
{Spry.Effect.setStyleProp(element,'position','relative');Spry.Effect.setStyleProp(element,'top','');Spry.Effect.setStyleProp(element,'left','');}}
if(firstChildElt)
{Spry.Effect.makePositioned(firstChildElt);Spry.Effect.makeClipping(firstChildElt);var childRect=Spry.Effect.getDimensionsRegardlessOfDisplayState(firstChildElt,element);Spry.Effect.setStyleProp(firstChildElt,'width',childRect.width+'px');}
var elementRect=Spry.Effect.getDimensionsRegardlessOfDisplayState(element);var startOffsetPosition=new Spry.Effect.Utils.Position();startOffsetPosition.x=parseInt(Spry.Effect.getStyleProp(firstChildElt,"left"));startOffsetPosition.y=parseInt(Spry.Effect.getStyleProp(firstChildElt,"top"));if(!startOffsetPosition.x)startOffsetPosition.x=0;if(!startOffsetPosition.y)startOffsetPosition.y=0;if(options&&options.horizontal!==null&&options.horizontal===true)
slideHorizontally=true;var movePx=slideHorizontally?elementRect.width:elementRect.height;var fromPos=new Spry.Effect.Utils.Position;fromPos.x=startOffsetPosition.x;fromPos.y=startOffsetPosition.y;var toPos=new Spry.Effect.Utils.Position;toPos.x=slideHorizontally?startOffsetPosition.x-movePx:startOffsetPosition.x;toPos.y=slideHorizontally?startOffsetPosition.y:startOffsetPosition.y-movePx;var optionFrom=options?options.from:elementRect.height;var optionTo=options?options.to:0;if(options)
{if(options.duration!=null)durationInMilliseconds=options.duration;if(options.from!=null)
{if(slideHorizontally)
{if(Spry.Effect.Utils.isPercentValue(options.from))
fromPos.x=movePx*Spry.Effect.Utils.getPercentValue(options.from)/100;else
fromPos.x=Spry.Effect.Utils.getPixelValue(options.from);}
else
{if(Spry.Effect.Utils.isPercentValue(options.from))
fromPos.y=movePx*Spry.Effect.Utils.getPercentValue(options.from)/100;else
fromPos.y=Spry.Effect.Utils.getPixelValue(options.from);}}
if(options.to!=null)
{if(slideHorizontally)
{if(Spry.Effect.Utils.isPercentValue(options.to))
toPos.x=movePx*Spry.Effect.Utils.getPercentValue(options.to)/100;else
toPos.x=Spry.Effect.Utils.getPixelValue(options.to);}
else
{if(Spry.Effect.Utils.isPercentValue(options.to))
toPos.y=movePx*Spry.Effect.Utils.getPercentValue(options.to)/100;else
toPos.y=Spry.Effect.Utils.getPixelValue(options.to);}}
if(options.toggle!=null)doToggle=options.toggle;if(options.transition!=null)kindOfTransition=options.transition;if(options.setup!=null)setupCallback=options.setup;if(options.finish!=null)finishCallback=options.finish;}
options={duration:durationInMilliseconds,toggle:doToggle,transition:kindOfTransition,setup:setupCallback,finish:finishCallback,from:optionFrom,to:optionTo};var slideEffect=new Spry.Effect.MoveSlide(element,fromPos,toPos,slideHorizontally,options);slideEffect.name='Slide';var registeredEffect=SpryRegistry.getRegisteredEffect(element,slideEffect);registeredEffect.start();return registeredEffect;};Spry.Effect.GrowShrink=function(element,options)
{var element=Spry.Effect.getElement(element);Spry.Effect.makePositioned(element);Spry.Effect.makeClipping(element);var startOffsetPosition=new Spry.Effect.Utils.Position();startOffsetPosition.x=parseInt(Spry.Effect.getStylePropRegardlessOfDisplayState(element,"left"));startOffsetPosition.y=parseInt(Spry.Effect.getStylePropRegardlessOfDisplayState(element,"top"));if(!startOffsetPosition.x)startOffsetPosition.x=0;if(!startOffsetPosition.y)startOffsetPosition.y=0;var dimRect=Spry.Effect.getDimensionsRegardlessOfDisplayState(element);var originalWidth=dimRect.width;var originalHeight=dimRect.height;var propFactor=(originalWidth==0)?1:originalHeight/originalWidth;var durationInMilliseconds=500;var doToggle=false;var kindOfTransition=Spry.sinusoidalTransition;var fromRect=new Spry.Effect.Utils.Rectangle;fromRect.width=0;fromRect.height=0;var toRect=new Spry.Effect.Utils.Rectangle;toRect.width=originalWidth;toRect.height=originalHeight;var setupCallback=null;var finishCallback=null;var doScaleContent=true;var optionFrom=options?options.from:dimRect.width;var optionTo=options?options.to:0;var calcHeight=false;var growFromCenter=true;if(options)
{if(options.referHeight!=null)calcHeight=options.referHeight;if(options.growCenter!=null)growFromCenter=options.growCenter;if(options.duration!=null)durationInMilliseconds=options.duration;if(options.from!=null)
{if(Spry.Effect.Utils.isPercentValue(options.from))
{fromRect.width=originalWidth*(Spry.Effect.Utils.getPercentValue(options.from)/100);fromRect.height=originalHeight*(Spry.Effect.Utils.getPercentValue(options.from)/100);}
else
{if(calcHeight)
{fromRect.height=Spry.Effect.Utils.getPixelValue(options.from);fromRect.width=Spry.Effect.Utils.getPixelValue(options.from)/propFactor;}
else
{fromRect.width=Spry.Effect.Utils.getPixelValue(options.from);fromRect.height=propFactor*Spry.Effect.Utils.getPixelValue(options.from);}}}
if(options.to!=null)
{if(Spry.Effect.Utils.isPercentValue(options.to))
{toRect.width=originalWidth*(Spry.Effect.Utils.getPercentValue(options.to)/100);toRect.height=originalHeight*(Spry.Effect.Utils.getPercentValue(options.to)/100);}
else
{if(calcHeight)
{toRect.height=Spry.Effect.Utils.getPixelValue(options.to);toRect.width=Spry.Effect.Utils.getPixelValue(options.to)/propFactor;}
else
{toRect.width=Spry.Effect.Utils.getPixelValue(options.to);toRect.height=propFactor*Spry.Effect.Utils.getPixelValue(options.to);}}}
if(options.toggle!=null)doToggle=options.toggle;if(options.transition!=null)kindOfTransition=options.transition;if(options.setup!=null)setupCallback=options.setup;if(options.finish!=null)finishCallback=options.finish;}
options={duration:durationInMilliseconds,toggle:doToggle,transition:kindOfTransition,scaleContent:doScaleContent,from:optionFrom,to:optionTo};var effectCluster=new Spry.Effect.Cluster({toggle:doToggle,setup:setupCallback,finish:finishCallback});effectCluster.name='GrowShrink';var sizeEffect=new Spry.Effect.Size(element,fromRect,toRect,options);effectCluster.addParallelEffect(sizeEffect);if(growFromCenter)
{options={duration:durationInMilliseconds,toggle:doToggle,transition:kindOfTransition,from:optionFrom,to:optionTo};var fromPos=new Spry.Effect.Utils.Position;fromPos.x=startOffsetPosition.x+(originalWidth-fromRect.width)/2.0;fromPos.y=startOffsetPosition.y+(originalHeight-fromRect.height)/2.0;var toPos=new Spry.Effect.Utils.Position;toPos.x=startOffsetPosition.x+(originalWidth-toRect.width)/2.0;toPos.y=startOffsetPosition.y+(originalHeight-toRect.height)/2.0;var initialProps2={top:fromPos.y,left:fromPos.x};var moveEffect=new Spry.Effect.Move(element,fromPos,toPos,options,initialProps2);effectCluster.addParallelEffect(moveEffect);}
var registeredEffect=SpryRegistry.getRegisteredEffect(element,effectCluster);registeredEffect.start();return registeredEffect;};Spry.Effect.Shake=function(element,options)
{var element=Spry.Effect.getElement(element);Spry.Effect.makePositioned(element);var setupCallback=null;var finishCallback=null;if(options)
{if(options.setup!=null)setupCallback=options.setup;if(options.finish!=null)finishCallback=options.finish;}
var startOffsetPosition=new Spry.Effect.Utils.Position();startOffsetPosition.x=parseInt(Spry.Effect.getStyleProp(element,"left"));startOffsetPosition.y=parseInt(Spry.Effect.getStyleProp(element,"top"));if(!startOffsetPosition.x)startOffsetPosition.x=0;if(!startOffsetPosition.y)startOffsetPosition.y=0;var shakeEffectCluster=new Spry.Effect.Cluster({setup:setupCallback,finish:finishCallback});shakeEffectCluster.name='Shake';var fromPos=new Spry.Effect.Utils.Position;fromPos.x=startOffsetPosition.x+0;fromPos.y=startOffsetPosition.y+0;var toPos=new Spry.Effect.Utils.Position;toPos.x=startOffsetPosition.x+20;toPos.y=startOffsetPosition.y+0;options={duration:50,toggle:false};var effect=new Spry.Effect.Move(element,fromPos,toPos,options);shakeEffectCluster.addNextEffect(effect);var fromPos=new Spry.Effect.Utils.Position;fromPos.x=startOffsetPosition.x+20;fromPos.y=startOffsetPosition.y+0;var toPos=new Spry.Effect.Utils.Position;toPos.x=startOffsetPosition.x+-20;toPos.y=startOffsetPosition.y+0;options={duration:100,toggle:false};var effect=new Spry.Effect.Move(element,fromPos,toPos,options);shakeEffectCluster.addNextEffect(effect);var fromPos=new Spry.Effect.Utils.Position;fromPos.x=startOffsetPosition.x+-20;fromPos.y=startOffsetPosition.y+0;var toPos=new Spry.Effect.Utils.Position;toPos.x=startOffsetPosition.x+20;toPos.y=startOffsetPosition.y+0;options={duration:100,toggle:false};var effect=new Spry.Effect.Move(element,fromPos,toPos,options);shakeEffectCluster.addNextEffect(effect);var fromPos=new Spry.Effect.Utils.Position;fromPos.x=startOffsetPosition.x+20;fromPos.y=startOffsetPosition.y+0;var toPos=new Spry.Effect.Utils.Position;toPos.x=startOffsetPosition.x+-20;toPos.y=startOffsetPosition.y+0;options={duration:100,toggle:false};var effect=new Spry.Effect.Move(element,fromPos,toPos,options);shakeEffectCluster.addNextEffect(effect);var fromPos=new Spry.Effect.Utils.Position;fromPos.x=startOffsetPosition.x+-20;fromPos.y=startOffsetPosition.y+0;var toPos=new Spry.Effect.Utils.Position;toPos.x=startOffsetPosition.x+20;toPos.y=startOffsetPosition.y+0;options={duration:100,toggle:false};var effect=new Spry.Effect.Move(element,fromPos,toPos,options);shakeEffectCluster.addNextEffect(effect);var fromPos=new Spry.Effect.Utils.Position;fromPos.x=startOffsetPosition.x+20;fromPos.y=startOffsetPosition.y+0;var toPos=new Spry.Effect.Utils.Position;toPos.x=startOffsetPosition.x+0;toPos.y=startOffsetPosition.y+0;options={duration:50,toggle:false};var effect=new Spry.Effect.Move(element,fromPos,toPos,options);shakeEffectCluster.addNextEffect(effect);var registeredEffect=SpryRegistry.getRegisteredEffect(element,shakeEffectCluster);registeredEffect.start();return registeredEffect;}
Spry.Effect.Squish=function(element,options)
{var element=Spry.Effect.getElement(element);var durationInMilliseconds=500;var doToggle=true;var setupCallback=null;var finishCallback=null;if(options)
{if(options.duration!=null)durationInMilliseconds=options.duration;if(options.toggle!=null)doToggle=options.toggle;if(options.setup!=null)setupCallback=options.setup;if(options.finish!=null)finishCallback=options.finish;}
Spry.Effect.makePositioned(element);Spry.Effect.makeClipping(element);var originalRect=Spry.Effect.getDimensionsRegardlessOfDisplayState(element);var startWidth=originalRect.width;var startHeight=originalRect.height;var stopWidth=0;var stopHeight=0;var fromRect=new Spry.Effect.Utils.Rectangle;fromRect.width=startWidth;fromRect.height=startHeight;var toRect=new Spry.Effect.Utils.Rectangle;toRect.width=stopWidth;toRect.height=stopHeight;var doScaleContent=true;options={duration:durationInMilliseconds,toggle:doToggle,scaleContent:doScaleContent,setup:setupCallback,finish:finishCallback};var squishEffect=new Spry.Effect.Size(element,fromRect,toRect,options);squishEffect.name='Squish';var registeredEffect=SpryRegistry.getRegisteredEffect(element,squishEffect);registeredEffect.start();return registeredEffect;};Spry.Effect.Pulsate=function(element,options)
{var element=Spry.Effect.getElement(element);var durationInMilliseconds=400;var fromOpacity=100.0;var toOpacity=0.0;var doToggle=false;var kindOfTransition=Spry.linearTransition;var setupCallback=null;var finishCallback=null;if(options)
{if(options.duration!=null)durationInMilliseconds=options.duration;if(options.from!=null)fromOpacity=options.from;if(options.to!=null)toOpacity=options.to;if(options.toggle!=null)doToggle=options.toggle;if(options.transition!=null)kindOfTransition=options.transition;if(options.setup!=null)setupCallback=options.setup;if(options.finish!=null)finishCallback=options.finish;}
options={duration:durationInMilliseconds,toggle:doToggle,transition:kindOfTransition,setup:setupCallback,finish:finishCallback};fromOpacity=fromOpacity/100.0;toOpacity=toOpacity/100.0;var pulsateEffectCluster=new Spry.Effect.Cluster();var fadeEffect=new Spry.Effect.Opacity(element,fromOpacity,toOpacity,options);var appearEffect=new Spry.Effect.Opacity(element,toOpacity,fromOpacity,options);pulsateEffectCluster.addNextEffect(fadeEffect);pulsateEffectCluster.addNextEffect(appearEffect);pulsateEffectCluster.addNextEffect(fadeEffect);pulsateEffectCluster.addNextEffect(appearEffect);pulsateEffectCluster.addNextEffect(fadeEffect);pulsateEffectCluster.addNextEffect(appearEffect);pulsateEffectCluster.name='Pulsate';var registeredEffect=SpryRegistry.getRegisteredEffect(element,pulsateEffectCluster);registeredEffect.start();return registeredEffect;};Spry.Effect.Puff=function(element,options)
{var element=Spry.Effect.getElement(element);Spry.Effect.makePositioned(element);var setupCallback=null;var finishCallback=null;if(options)
{if(options.setup!=null)setupCallback=options.setup;if(options.finish!=null)finishCallback=options.finish;}
var puffEffectCluster=new Spry.Effect.Cluster;var durationInMilliseconds=500;var originalRect=Spry.Effect.getDimensions(element);var startWidth=originalRect.width;var startHeight=originalRect.height;var stopWidth=startWidth*2;var stopHeight=startHeight*2;var fromRect=new Spry.Effect.Utils.Rectangle;fromRect.width=startWidth;fromRect.height=startHeight;var toRect=new Spry.Effect.Utils.Rectangle;toRect.width=stopWidth;toRect.height=stopHeight;var doScaleContent=false;options={duration:durationInMilliseconds,toggle:false,scaleContent:doScaleContent};var sizeEffect=new Spry.Effect.Size(element,fromRect,toRect,options);puffEffectCluster.addParallelEffect(sizeEffect);options={duration:durationInMilliseconds,toggle:false};var fromOpacity=1.0;var toOpacity=0.0;var opacityEffect=new Spry.Effect.Opacity(element,fromOpacity,toOpacity,options);puffEffectCluster.addParallelEffect(opacityEffect);options={duration:durationInMilliseconds,toggle:false};var fromPos=new Spry.Effect.Utils.Position;fromPos.x=0;fromPos.y=0;var toPos=new Spry.Effect.Utils.Position;toPos.x=startWidth/2.0*-1.0;toPos.y=startHeight/2.0*-1.0;var moveEffect=new Spry.Effect.Move(element,fromPos,toPos,options);puffEffectCluster.addParallelEffect(moveEffect);puffEffectCluster.setup=setupCallback;puffEffectCluster.finish=finishCallback;puffEffectCluster.name='Puff';var registeredEffect=SpryRegistry.getRegisteredEffect(element,puffEffectCluster);registeredEffect.start();return registeredEffect;};Spry.Effect.DropOut=function(element,options)
{var element=Spry.Effect.getElement(element);var dropoutEffectCluster=new Spry.Effect.Cluster;var durationInMilliseconds=500;Spry.Effect.makePositioned(element);var setupCallback=null;var finishCallback=null;if(options)
{if(options.setup!=null)setupCallback=options.setup;if(options.finish!=null)finishCallback=options.finish;}
var startOffsetPosition=new Spry.Effect.Utils.Position();startOffsetPosition.x=parseInt(Spry.Effect.getStyleProp(element,"left"));startOffsetPosition.y=parseInt(Spry.Effect.getStyleProp(element,"top"));if(!startOffsetPosition.x)startOffsetPosition.x=0;if(!startOffsetPosition.y)startOffsetPosition.y=0;var fromPos=new Spry.Effect.Utils.Position;fromPos.x=startOffsetPosition.x+0;fromPos.y=startOffsetPosition.y+0;var toPos=new Spry.Effect.Utils.Position;toPos.x=startOffsetPosition.x+0;toPos.y=startOffsetPosition.y+160;options={from:fromPos,to:toPos,duration:durationInMilliseconds,toggle:true};var moveEffect=new Spry.Effect.Move(element,options.from,options.to,options);dropoutEffectCluster.addParallelEffect(moveEffect);options={duration:durationInMilliseconds,toggle:true};var fromOpacity=1.0;var toOpacity=0.0;var opacityEffect=new Spry.Effect.Opacity(element,fromOpacity,toOpacity,options);dropoutEffectCluster.addParallelEffect(opacityEffect);dropoutEffectCluster.setup=setupCallback;dropoutEffectCluster.finish=finishCallback;dropoutEffectCluster.name='DropOut';var registeredEffect=SpryRegistry.getRegisteredEffect(element,dropoutEffectCluster);registeredEffect.start();return registeredEffect;};Spry.Effect.Fold=function(element,options)
{var element=Spry.Effect.getElement(element);var durationInMilliseconds=1000;var doToggle=false;var doScaleContent=true;var foldEffectCluster=new Spry.Effect.Cluster();var originalRect=Spry.Effect.getDimensions(element);var startWidth=originalRect.width;var startHeight=originalRect.height;var stopWidth=startWidth;var stopHeight=startHeight/5;var fromRect=new Spry.Effect.Utils.Rectangle;fromRect.width=startWidth;fromRect.height=startHeight;var toRect=new Spry.Effect.Utils.Rectangle;toRect.width=stopWidth;toRect.height=stopHeight;options={duration:durationInMilliseconds,toggle:doToggle,scaleContent:doScaleContent};var sizeEffect=new Spry.Effect.Size(element,fromRect,toRect,options);foldEffectCluster.addNextEffect(sizeEffect);durationInMilliseconds=500;options={duration:durationInMilliseconds,toggle:doToggle,scaleContent:doScaleContent};fromRect.width="100%";fromRect.height="20%";toRect.width="10%";toRect.height="20%";var sizeEffect=new Spry.Effect.Size(element,fromRect,toRect,options);foldEffectCluster.addNextEffect(sizeEffect);foldEffectCluster.name='Fold';var registeredEffect=SpryRegistry.getRegisteredEffect(element,foldEffectCluster);registeredEffect.start();return registeredEffect;};Spry.Effect.DoFade=function(element,options)
{return Spry.Effect.AppearFade(element,options);};Spry.Effect.DoBlind=function(element,options)
{return Spry.Effect.Blind(element,options);};Spry.Effect.DoHighlight=function(element,options)
{return Spry.Effect.Highlight(element,options);};Spry.Effect.DoSlide=function(element,options)
{return Spry.Effect.Slide(element,options);};Spry.Effect.DoGrow=function(element,options)
{return Spry.Effect.GrowShrink(element,options);};Spry.Effect.DoShake=function(element,options)
{return Spry.Effect.Shake(element,options);};Spry.Effect.DoSquish=function(element,options)
{return Spry.Effect.Squish(element,options);};Spry.Effect.DoPulsate=function(element,options)
{return Spry.Effect.Pulsate(element,options);};Spry.Effect.DoPuff=function(element,options)
{return Spry.Effect.Puff(element,options);};Spry.Effect.DoDropOut=function(element,options)
{return Spry.Effect.DropOut(element,options);};Spry.Effect.DoFold=function(element,options)
{return Spry.Effect.Fold(element,options);};