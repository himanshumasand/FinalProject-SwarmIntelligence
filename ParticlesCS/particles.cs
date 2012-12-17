#version 430 compatibility
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object : enable

struct pos
{
	vec4 pxyzw;	// positions
};

struct vel
{
	vec4 vxyzw;	// velocities
};

struct color
{
	vec4 crgba;	// colors
};

struct svel
{
	vec4 svxyzw;	// velocities
};

layout( std140, binding=4 ) buffer Pos {
	struct pos  Positions[  ];		// array of structures
};

layout( std140, binding=5 ) buffer Vel {
	struct vel  Velocities[  ];		// array of structures
};

layout( std140, binding=6 ) buffer Col {
	struct color  Colors[  ];		// array of structures
};

//layout( std140, binding=7 ) buffer SVel {
//    struct svel  SVelocities[  ];		// array of structures
//};

layout( local_size_x = 1024,  local_size_y = 1, local_size_z = 1 )   in;



uniform float u_maxParticles;

uniform float u_xMouse;
uniform float u_yMouse;
uniform float u_zMouse;

uniform float u_xcm;
uniform float u_ycm;
uniform float u_zcm;

uniform float u_xalign;
uniform float u_yalign;
uniform float u_zalign;


void
main( )
{
    uint  gid = gl_GlobalInvocationID.x;	// the .y and .z are both 1 in this case
    //gl_GlobalInvocationID = gl_WorkGroupID * gl_WorkGroupSize + gl_LocalInvocationID
	
    vec3 p  = Positions[  gid  ].pxyzw.xyz;
    vec3 v  = Velocities[ gid  ].vxyzw.xyz;
    
    vec3 G = vec3(0., 0., 0.);
	float DT = 0.2;
	

    //Cohesion
    vec3 centerOfMass = vec3(u_xcm, u_ycm, u_zcm);
    centerOfMass = (centerOfMass - p)/(u_maxParticles - 1);
    vec3 cohesionVel = centerOfMass - p;


    //Alignment
    vec3 alignVel = vec3(u_xalign, u_yalign, u_zalign);


    //Separation

    vec3 separationVel = vec3(0., 0., 0.);
	
    //vec3 dist = vec3(0., 0., 0.);

    //for(int i = 0; i < u_maxParticles; i++)
    //{
    //    if(i != gid)
    //    {
    //        dist = Positions[gid].pxyzw.xyz - Positions[i].pxyzw.xyz;
    //        separationVel += dist / (length(dist)*length(dist));
    //    }
    //}

	//vec3 separationVel = SVelocities[gid].svxyzw.xyz;

    //Line Following

    vec3 lineVel = vec3(0., 0., 0.);

    if(gid != 0)
        lineVel = Positions[gid-1].pxyzw.xyz - Positions[gid].pxyzw.xyz;
    else
        lineVel = vec3(u_xMouse, u_yMouse, u_zMouse) - Positions[gid].pxyzw.xyz;


    

    vec3 flockVel = 0.01 * cohesionVel + 0.0 * alignVel + 1.0 * vec3(separationVel.x, separationVel.y, separationVel.z) + 0.01 * ( vec3(u_xMouse, u_yMouse, u_zMouse) - p);

    vec3 vp = 0.99 * v + 0.0 * flockVel + 0.1 * lineVel;
    if(length(vp) > 30)
    {
        vp = vp/length(vp) * 30.0;
    }
    vec3 pp = p + vp*DT + .5*DT*DT*G;
    


    

		
    Positions[  gid  ].pxyzw     = vec4( pp, 1. );
	// Positions[  gid  ].pxyzw.xyz = pp;
	Velocities[ gid  ].vxyzw.xyz = vp;
}
