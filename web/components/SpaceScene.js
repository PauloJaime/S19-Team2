import React, { Component } from 'react';
import * as THREE from '../three/three';
import LensFlare from '../three/LensFlare'
import OrbitControls from '../three/OrbitControls';
import { 
  addLighting,
  buildScene, 
  addObjects,
  addAxisHelper
} from './sceneHelper';

const earthScale = 4;
const moonScale = 3.5;
const moonOrbitRadius = 10;
const earthOrbitRadius = 930;
const axis = new THREE.Vector3(0, 0.4101524, 0).normalize();

export default class SpaceScene extends Component {

  constructor(props) {
    super(props);
    this.state = {
      earth: {},
      moon: {},
      pointLight: {}
    };
  }

  /**
   * componentDidMount
   * Lifecycle method in React
   * Gets called everytime the component (page) loads
   */
  componentDidMount() {
    
    // Build base scene objects
    let { scene, camera, controls, renderer } = buildScene();

    // Add lighting, objects, and axis helper
    addLighting(scene).then(pointLighting => this.setState({ pointLight: pointLighting }));

    addObjects(scene, earthScale, moonScale).then(({ earthObj, moonObj }) => {
      this.setState({ earth: earthObj });
      this.setState({ moon: moonObj });
    }).then(() => {
      animate();
    });

    addAxisHelper(scene);


    /**
     * Update function
     * Runs every frame to animate the scene
     */
    const update = () => {
      let date = Date.now() * 0.00001;

      this.state.pointLight.position.x = this.state.earth.position.x + Math.sin(date) * earthOrbitRadius;
      this.state.pointLight.position.z = this.state.earth.position.z + Math.cos(date) * earthOrbitRadius;

      this.state.moon.position.x = this.state.earth.position.x + Math.sin(date * 3) * moonOrbitRadius;
      this.state.moon.position.z = this.state.earth.position.z + Math.cos(date * 3) * moonOrbitRadius;

      this.state.earth.rotateOnAxis(axis, 0.0009);
      this.state.moon.rotateOnAxis(axis, 0.001);
    };

    /**
     * Render function
     * sends scene and camera props to renderer
     */
    const render = () => {
      renderer.render(scene, camera);
    };

    /**
     * Animate function
     * gets new frame, updates, and renders scene
     */
    const animate = () => {
      requestAnimationFrame(animate);
      update();
      render();
    };
  }

  render() {
    return(
      <div className="space-scene"
        ref={(mount) => { this.mount = mount }}
      />
    )
  }
}