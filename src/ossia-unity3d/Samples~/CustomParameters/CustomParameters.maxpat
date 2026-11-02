{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0,
			"architecture": "x64",
			"modernui": 1
		},
		"classnamespace": "box",
		"rect": [
			100,
			100,
			740,
			750
		],
		"boxes": [
			{
				"box": {
					"id": "title",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20,
						15,
						600,
						20
					],
					"text": "Custom parameters: drive and follow the five fields of the Unity sample",
					"fontface": 1
				}
			},
			{
				"box": {
					"id": "how",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20,
						37,
						700,
						20
					],
					"text": "In Unity, Play the scene CustomParameters, then click connect unity. Left: what Max sends. Right: what Unity sends back."
				}
			},
			{
				"box": {
					"id": "cn",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"patching_rect": [
						20,
						70,
						90,
						22
					],
					"text": "connect unity",
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "cu",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"patching_rect": [
						120,
						70,
						230,
						22
					],
					"text": "connect oscquery ws://127.0.0.1:5678",
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "dc",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"patching_rect": [
						360,
						70,
						75,
						22
					],
					"text": "disconnect",
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "cl",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 3,
					"patching_rect": [
						20,
						105,
						120,
						22
					],
					"text": "ossia.client custom",
					"outlettype": [
						"",
						"",
						""
					]
				}
			},
			{
				"box": {
					"id": "pc",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20,
						135,
						80,
						22
					],
					"text": "print client",
					"outlettype": []
				}
			},
			{
				"box": {
					"id": "c_speed",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20,
						180,
						280,
						20
					],
					"text": "speed (float, degrees per second)"
				}
			},
			{
				"box": {
					"id": "u_speed",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20,
						205,
						60,
						22
					],
					"outlettype": [
						"",
						"bang"
					],
					"minimum": -360.0,
					"maximum": 360.0
				}
			},
			{
				"box": {
					"id": "r_speed",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 3,
					"patching_rect": [
						20,
						235,
						260,
						22
					],
					"text": "ossia.remote /Shape/CustomParameters/speed",
					"outlettype": [
						"",
						"",
						""
					]
				}
			},
			{
				"box": {
					"id": "s_speed",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						330,
						235,
						75,
						22
					],
					"text": "prepend set",
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "v_speed",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"patching_rect": [
						420,
						235,
						180,
						22
					],
					"text": "",
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "c_axis",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20,
						280,
						280,
						20
					],
					"text": "axis (int, 0 x, 1 y, 2 z)"
				}
			},
			{
				"box": {
					"id": "u_axis",
					"maxclass": "number",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20,
						305,
						50,
						22
					],
					"outlettype": [
						"",
						"bang"
					],
					"minimum": 0,
					"maximum": 2
				}
			},
			{
				"box": {
					"id": "r_axis",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 3,
					"patching_rect": [
						20,
						335,
						260,
						22
					],
					"text": "ossia.remote /Shape/CustomParameters/axis",
					"outlettype": [
						"",
						"",
						""
					]
				}
			},
			{
				"box": {
					"id": "s_axis",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						330,
						335,
						75,
						22
					],
					"text": "prepend set",
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "v_axis",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"patching_rect": [
						420,
						335,
						180,
						22
					],
					"text": "",
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "c_spin",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20,
						380,
						280,
						20
					],
					"text": "spin (bool)"
				}
			},
			{
				"box": {
					"id": "u_spin",
					"maxclass": "toggle",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						20,
						405,
						24,
						24
					],
					"outlettype": [
						"int"
					],
					"parameter_enable": 0
				}
			},
			{
				"box": {
					"id": "r_spin",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 3,
					"patching_rect": [
						20,
						435,
						260,
						22
					],
					"text": "ossia.remote /Shape/CustomParameters/spin",
					"outlettype": [
						"",
						"",
						""
					]
				}
			},
			{
				"box": {
					"id": "s_spin",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						330,
						435,
						75,
						22
					],
					"text": "prepend set",
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "v_spin",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"patching_rect": [
						420,
						435,
						180,
						22
					],
					"text": "",
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "c_color",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20,
						480,
						280,
						20
					],
					"text": "color (Color, r g b a from 0 to 1)"
				}
			},
			{
				"box": {
					"id": "u_color0",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20,
						505,
						50,
						22
					],
					"outlettype": [
						"",
						"bang"
					],
					"minimum": 0.0
				}
			},
			{
				"box": {
					"id": "u_color1",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						75,
						505,
						50,
						22
					],
					"outlettype": [
						"",
						"bang"
					],
					"minimum": 0.0
				}
			},
			{
				"box": {
					"id": "u_color2",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						130,
						505,
						50,
						22
					],
					"outlettype": [
						"",
						"bang"
					],
					"minimum": 0.0
				}
			},
			{
				"box": {
					"id": "u_color3",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						185,
						505,
						50,
						22
					],
					"outlettype": [
						"",
						"bang"
					],
					"minimum": 0.0
				}
			},
			{
				"box": {
					"id": "p_color",
					"maxclass": "newobj",
					"numinlets": 4,
					"numoutlets": 1,
					"patching_rect": [
						20,
						532,
						215,
						22
					],
					"text": "pak 1. 0.5 0. 1.",
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "r_color",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 3,
					"patching_rect": [
						20,
						560,
						260,
						22
					],
					"text": "ossia.remote /Shape/CustomParameters/color",
					"outlettype": [
						"",
						"",
						""
					]
				}
			},
			{
				"box": {
					"id": "s_color",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						330,
						560,
						75,
						22
					],
					"text": "prepend set",
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "v_color",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"patching_rect": [
						420,
						560,
						180,
						22
					],
					"text": "",
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "c_size",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20,
						605,
						280,
						20
					],
					"text": "size (Vector3, x y z)"
				}
			},
			{
				"box": {
					"id": "u_size0",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20,
						630,
						50,
						22
					],
					"outlettype": [
						"",
						"bang"
					],
					"minimum": 0.0
				}
			},
			{
				"box": {
					"id": "u_size1",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						75,
						630,
						50,
						22
					],
					"outlettype": [
						"",
						"bang"
					],
					"minimum": 0.0
				}
			},
			{
				"box": {
					"id": "u_size2",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						130,
						630,
						50,
						22
					],
					"outlettype": [
						"",
						"bang"
					],
					"minimum": 0.0
				}
			},
			{
				"box": {
					"id": "p_size",
					"maxclass": "newobj",
					"numinlets": 3,
					"numoutlets": 1,
					"patching_rect": [
						20,
						657,
						160,
						22
					],
					"text": "pak 1. 1. 1.",
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "r_size",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 3,
					"patching_rect": [
						20,
						685,
						260,
						22
					],
					"text": "ossia.remote /Shape/CustomParameters/size",
					"outlettype": [
						"",
						"",
						""
					]
				}
			},
			{
				"box": {
					"id": "s_size",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						330,
						685,
						75,
						22
					],
					"text": "prepend set",
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "v_size",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"patching_rect": [
						420,
						685,
						180,
						22
					],
					"text": "",
					"outlettype": [
						""
					]
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"source": [
						"cn",
						0
					],
					"destination": [
						"cl",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"cu",
						0
					],
					"destination": [
						"cl",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"dc",
						0
					],
					"destination": [
						"cl",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"cl",
						0
					],
					"destination": [
						"pc",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"u_speed",
						0
					],
					"destination": [
						"r_speed",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"r_speed",
						0
					],
					"destination": [
						"s_speed",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"s_speed",
						0
					],
					"destination": [
						"v_speed",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"u_axis",
						0
					],
					"destination": [
						"r_axis",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"r_axis",
						0
					],
					"destination": [
						"s_axis",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"s_axis",
						0
					],
					"destination": [
						"v_axis",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"u_spin",
						0
					],
					"destination": [
						"r_spin",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"r_spin",
						0
					],
					"destination": [
						"s_spin",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"s_spin",
						0
					],
					"destination": [
						"v_spin",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"u_color0",
						0
					],
					"destination": [
						"p_color",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"u_color1",
						0
					],
					"destination": [
						"p_color",
						1
					]
				}
			},
			{
				"patchline": {
					"source": [
						"u_color2",
						0
					],
					"destination": [
						"p_color",
						2
					]
				}
			},
			{
				"patchline": {
					"source": [
						"u_color3",
						0
					],
					"destination": [
						"p_color",
						3
					]
				}
			},
			{
				"patchline": {
					"source": [
						"p_color",
						0
					],
					"destination": [
						"r_color",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"r_color",
						0
					],
					"destination": [
						"s_color",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"s_color",
						0
					],
					"destination": [
						"v_color",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"u_size0",
						0
					],
					"destination": [
						"p_size",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"u_size1",
						0
					],
					"destination": [
						"p_size",
						1
					]
				}
			},
			{
				"patchline": {
					"source": [
						"u_size2",
						0
					],
					"destination": [
						"p_size",
						2
					]
				}
			},
			{
				"patchline": {
					"source": [
						"p_size",
						0
					],
					"destination": [
						"r_size",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"r_size",
						0
					],
					"destination": [
						"s_size",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"s_size",
						0
					],
					"destination": [
						"v_size",
						0
					]
				}
			}
		]
	}
}