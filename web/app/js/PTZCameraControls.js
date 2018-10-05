
// This set of controls performs pan, tilt and zoom. 
//
//    Pan - left mouse left and right / touch: one finger move
//    Tilt - left mouse up and down / touch: one finger move
//    Zoom - middle mouse, or mousewheel / touch: two finger spread or squish

THREE.PTZCameraControls = function ( camera, domElement ) {

  camera.rotation.order = 'YXZ'; 
	this.camera = camera;

	this.domElement = ( domElement !== undefined ) ? domElement : document;

	// Set to false to disable this control
	this.enabled = true;
  
  // Set true to translate instead of rotate in pan / tilt
  this.translate = false;

	// How far you can tilt vertically, upper and lower limits.
	// If set, must be a sub-interval of the interval [ -Math.PI/2, Math.PI/2 ].
	this.minTilt = -Math.PI/2; // radians
	this.maxTilt = Math.PI/2; // radians

	// How far you can zoom in and out
	this.minZoom = 0;
	this.maxZoom = Infinity;

	// Set to true to enable damping (inertia)
	this.enableDamping = false;
	this.dampingFactor = 0.25;

	// Set to false to disable panning
	this.enablePan = true;
	this.panSpeed = 1.0;

	// Set to false to disable tilting
	this.enableTilt = true;
	this.tiltSpeed = 1.0;

	// Set to false to disable zooming
	this.enableZoom = true;
	this.zoomSpeed = 1.0;

	// Mouse buttons
	this.mouseButtons = { PAN_TILT: THREE.MOUSE.LEFT, ZOOM: THREE.MOUSE.MIDDLE, SELECT: THREE.MOUSE.RIGHT };

	//
	// public methods
	//
	this.dispose = function () {

		scope.domElement.removeEventListener( 'contextmenu', onContextMenu, false );
		scope.domElement.removeEventListener( 'mousedown', onMouseDown, false );
		scope.domElement.removeEventListener( 'wheel', onMouseWheel, false );

		scope.domElement.removeEventListener( 'touchstart', onTouchStart, false );
		scope.domElement.removeEventListener( 'touchend', onTouchEnd, false );
		scope.domElement.removeEventListener( 'touchmove', onTouchMove, false );

		document.removeEventListener( 'mousemove', onMouseMove, false );
		document.removeEventListener( 'mouseup', onMouseUp, false );

	};

	//
	// internals
	//

	var scope = this;

	var selectEvent = { type: 'select' };
  var changeEvent = { type: 'change' };
  var rotateEvent = { type: 'rotate' };

	var STATE = { NONE: - 1, PAN_TILT: 0, ZOOM: 1, TOUCH_PAN_TILT: 2, TOUCH_ZOOM: 3 };

	var state = STATE.NONE;

	var rotateStart = new THREE.Vector2();
	var rotateEnd = new THREE.Vector2();
	var rotateDelta = new THREE.Vector2();

	var zoomStart = new THREE.Vector2();
	var zoomEnd = new THREE.Vector2();
	var zoomDelta = new THREE.Vector2();

	function getZoomScale() {
		return Math.pow( 0.95, scope.zoomSpeed );
	}

	function rotateLeft( angle ) {
    if (scope.translate){
      scope.camera.position.x -= angle;
    } else {
		  scope.camera.rotation.y -= angle;
    }
    scope.dispatchEvent( changeEvent );
	}

	function rotateUp( angle ) {
    if (scope.translate){
      scope.camera.position.y += angle;
    } else {
  	  scope.camera.rotation.x = Math.max( scope.minTilt, Math.min(scope.maxTilt, scope.camera.rotation.x - angle));
    }
    scope.dispatchEvent( changeEvent );
	}

	function zoomIn( zoomScale ) {
  	scope.camera.zoom = Math.max( scope.minZoom, Math.min( scope.maxZoom, scope.camera.zoom * zoomScale ) );
		scope.camera.updateProjectionMatrix();
    scope.dispatchEvent( changeEvent );
	}

	function zoomOut( zoomScale ) {
		scope.camera.zoom = Math.max( scope.minZoom, Math.min( scope.maxZoom, scope.camera.zoom / zoomScale ) );
		scope.camera.updateProjectionMatrix();
    scope.dispatchEvent( changeEvent );
	}

	//
	// event callbacks - update the camera state
	//

	function handleMouseDownRotate( event ) {
		//console.log( 'handleMouseDownRotate' );
		rotateStart.set( event.clientX, event.clientY );
	}

	function handleMouseDownZoom( event ) {
		//console.log( 'handleMouseDownZoom' );
		zoomStart.set( event.clientX, event.clientY );
	}

	function handleMouseMoveRotate( event ) {
		//console.log( 'handleMouseMoveRotate' );
		rotateEnd.set( event.clientX, event.clientY );
		rotateDelta.subVectors( rotateEnd, rotateStart );
		var element = scope.domElement === document ? scope.domElement.body : scope.domElement;
		// rotating across whole screen goes 360 degrees around
		rotateLeft( 2 * Math.PI * rotateDelta.x / element.clientWidth * scope.panSpeed );
		// rotating up and down along whole screen attempts to go 360, but limited to 180
		rotateUp( 2 * Math.PI * rotateDelta.y / element.clientHeight * scope.tiltSpeed );
		rotateStart.copy( rotateEnd );
	}

	function handleMouseMoveZoom( event ) {
		//console.log( 'handleMouseMoveZoom' );
		zoomEnd.set( event.clientX, event.clientY );
		zoomDelta.subVectors( zoomEnd, zoomStart );
		if ( zoomDelta.y > 0 ) {
			zoomIn( getZoomScale() );
		} else if ( zoomDelta.y < 0 ) {
			zoomOut( getZoomScale() );
		}
		zoomStart.copy( zoomEnd );
	}

	function handleMouseUp( event ) {
		// console.log( 'handleMouseUp' );
	}

	function handleMouseWheel( event ) {
		// console.log( 'handleMouseWheel' );
		if ( event.deltaY < 0 ) {
			zoomOut( getZoomScale() );
		} else if ( event.deltaY > 0 ) {
			zoomIn( getZoomScale() );
		}
	}

	function handleTouchStartRotate( event ) {
		//console.log( 'handleTouchStartRotate' );
		rotateStart.set( event.touches[ 0 ].pageX, event.touches[ 0 ].pageY );
	}

	function handleTouchStartZoom( event ) {
		//console.log( 'handleTouchStartZoom' );
		var dx = event.touches[ 0 ].pageX - event.touches[ 1 ].pageX;
		var dy = event.touches[ 0 ].pageY - event.touches[ 1 ].pageY;
		var distance = Math.sqrt( dx * dx + dy * dy );
		zoomStart.set( 0, distance );
	}

	function handleTouchMoveRotate( event ) {
		//console.log( 'handleTouchMoveRotate' );
		rotateEnd.set( event.touches[ 0 ].pageX, event.touches[ 0 ].pageY );
		rotateDelta.subVectors( rotateEnd, rotateStart );
		var element = scope.domElement === document ? scope.domElement.body : scope.domElement;
		// rotating across whole screen goes 360 degrees around
		rotateLeft( 2 * Math.PI * rotateDelta.x / element.clientWidth * scope.panSpeed );
		// rotating up and down along whole screen attempts to go 360, but limited to 180
		rotateUp( 2 * Math.PI * rotateDelta.y / element.clientHeight * scope.tiltSpeed );
		rotateStart.copy( rotateEnd );
	}

	function handleTouchMoveZoom( event ) {
		//console.log( 'handleTouchMoveZoom' );
		var dx = event.touches[ 0 ].pageX - event.touches[ 1 ].pageX;
		var dy = event.touches[ 0 ].pageY - event.touches[ 1 ].pageY;
		var distance = Math.sqrt( dx * dx + dy * dy );
		zoomEnd.set( 0, distance );
		zoomDelta.subVectors( zoomEnd, zoomStart );
		if ( zoomDelta.y > 0 ) {
			zoomOut( getZoomScale() );
		} else if ( zoomDelta.y < 0 ) {
			zoomIn( getZoomScale() );
		}
		zoomStart.copy( zoomEnd );
	}

	function handleTouchEnd( event ) {
		//console.log( 'handleTouchEnd' );
	}

	//
	// event handlers - FSM: listen for events and reset state
	//
	function onMouseDown( event ) {
		if ( scope.enabled === false ) return;
		event.preventDefault();
		switch ( event.button ) {
			case scope.mouseButtons.PAN_TILT:
				if ( scope.enableRotate === false ) return;
				handleMouseDownRotate( event );
				state = STATE.PAN_TILT;
				break;

			case scope.mouseButtons.ZOOM:
				if ( scope.enableZoom === false ) return;
				handleMouseDownZoom( event );
				state = STATE.ZOOM;
				break;
				
			case scope.mouseButtons.SELECT:
        state = STATE.NONE;
			  var rect = scope.domElement.getBoundingClientRect();
			  selectEvent.x = event.clientX - rect.left;
				selectEvent.y = event.clientY - rect.top;
			  scope.dispatchEvent( selectEvent );
			  break;
		}
		if ( state !== STATE.NONE ) {
			document.addEventListener( 'mousemove', onMouseMove, false );
			document.addEventListener( 'mouseup', onMouseUp, false );
		}

	}

	function onMouseMove( event ) {
		if ( scope.enabled === false ) return;
		event.preventDefault();
		switch ( state ) {
			case STATE.PAN_TILT:
				if ( scope.enableRotate === false ) return;
				handleMouseMoveRotate( event );
				break;

			case STATE.ZOOM:
				if ( scope.enableZoom === false ) return;
				handleMouseMoveZoom( event );
				break;
		}
	}

	function onMouseUp( event ) {
		if ( scope.enabled === false ) return;
		handleMouseUp( event );
		document.removeEventListener( 'mousemove', onMouseMove, false );
		document.removeEventListener( 'mouseup', onMouseUp, false );
		if (state == STATE.PAN_TILT) {
		  scope.dispatchEvent( rotateEvent );
		}
		state = STATE.NONE;
	}

	function onMouseWheel( event ) {
		if ( scope.enabled === false || scope.enableZoom === false || ( state !== STATE.NONE && state !== STATE.PAN_TILT ) ) return;
		event.preventDefault();
		event.stopPropagation();
		handleMouseWheel( event );
	}

	function onTouchStart( event ) {
		if ( scope.enabled === false ) return;
		switch ( event.touches.length ) {
			case 1:	// one-fingered touch: rotate
				if ( scope.enableRotate === false ) return;
				handleTouchStartRotate( event );
				state = STATE.TOUCH_PAN_TILT;
				break;

			case 2:	// two-fingered touch: zoom
				if ( scope.enableZoom === false ) return;
				handleTouchStartZoom( event );
				state = STATE.TOUCH_ZOOM;
				break;

			default:
				state = STATE.NONE;

		}
	}

	function onTouchMove( event ) {
		if ( scope.enabled === false ) return;
		event.preventDefault();
		event.stopPropagation();
		switch ( event.touches.length ) {
			case 1: // one-fingered touch: rotate
				if ( scope.enableRotate === false ) return;
				if ( state !== STATE.TOUCH_PAN_TILT ) return; // is this needed?...
				handleTouchMoveRotate( event );
				break;

			case 2: // two-fingered touch: zoom
				if ( scope.enableZoom === false ) return;
  			if ( state !== STATE.TOUCH_ZOOM ) return; // is this needed?...
				handleTouchMoveZoom( event );
				break;

			default:
				state = STATE.NONE;
		}
	}

	function onTouchEnd( event ) {
		if ( scope.enabled === false ) return;
		handleTouchEnd( event );
		state = STATE.NONE;
	}

	function onContextMenu( event ) {
		if ( scope.enabled === false ) return;
		event.preventDefault();
	}


	scope.domElement.addEventListener( 'contextmenu', onContextMenu, false );

	scope.domElement.addEventListener( 'mousedown', onMouseDown, false );
	scope.domElement.addEventListener( 'wheel', onMouseWheel, false );

	scope.domElement.addEventListener( 'touchstart', onTouchStart, false );
	scope.domElement.addEventListener( 'touchend', onTouchEnd, false );
	scope.domElement.addEventListener( 'touchmove', onTouchMove, false );

};

THREE.PTZCameraControls.prototype = Object.create( THREE.EventDispatcher.prototype );
THREE.PTZCameraControls.prototype.constructor = THREE.PTZCameraControls;

