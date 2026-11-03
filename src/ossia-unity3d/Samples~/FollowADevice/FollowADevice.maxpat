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
			760,
			760
		],
		"boxes": [
			{
				"box": {
					"id": "t1",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20,
						15,
						440,
						20
					],
					"text": "Follow a device: follow_me, the device the Unity sample FollowADevice follows"
				}
			},
			{
				"box": {
					"id": "t2",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20,
						37,
						590,
						20
					],
					"text": "Max exposes it as soon as the patch opens. In Unity, Play FollowADevice: the cube takes size, color and spin."
				}
			},
			{
				"box": {
					"id": "lm",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						20,
						70,
						220,
						22
					],
					"outlettype": [
						""
					],
					"text": "loadmess expose oscquery 9234 9678"
				}
			},
			{
				"box": {
					"id": "ex",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"patching_rect": [
						260,
						70,
						170,
						22
					],
					"outlettype": [
						""
					],
					"text": "expose oscquery 9234 9678"
				}
			},
			{
				"box": {
					"id": "dev",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						20,
						105,
						140,
						22
					],
					"outlettype": [
						""
					],
					"text": "ossia.device follow_me"
				}
			},
			{
				"box": {
					"id": "pd",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20,
						135,
						80,
						22
					],
					"outlettype": [],
					"text": "print device"
				}
			},
			{
				"box": {
					"id": "t3",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						200,
						105,
						360,
						20
					],
					"text": "Unity finds it by its name, or at ws://127.0.0.1:9678"
				}
			},
			{
				"box": {
					"id": "c1",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20,
						180,
						320,
						20
					],
					"text": "size (float, from 0.1 to 3)"
				}
			},
			{
				"box": {
					"id": "f1",
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
					]
				}
			},
			{
				"box": {
					"id": "p1",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20,
						235,
						385,
						22
					],
					"outlettype": [
						"",
						""
					],
					"text": "ossia.parameter cube/size @type float @min 0.1 @max 3. @default 1."
				}
			},
			{
				"box": {
					"id": "c2",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20,
						280,
						320,
						20
					],
					"text": "color (r g b a, from 0 to 1)"
				}
			},
			{
				"box": {
					"id": "f20",
					"maxclass": "flonum",
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
					]
				}
			},
			{
				"box": {
					"id": "f21",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						75,
						305,
						50,
						22
					],
					"outlettype": [
						"",
						"bang"
					]
				}
			},
			{
				"box": {
					"id": "f22",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						130,
						305,
						50,
						22
					],
					"outlettype": [
						"",
						"bang"
					]
				}
			},
			{
				"box": {
					"id": "f23",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						185,
						305,
						50,
						22
					],
					"outlettype": [
						"",
						"bang"
					]
				}
			},
			{
				"box": {
					"id": "pk",
					"maxclass": "newobj",
					"numinlets": 4,
					"numoutlets": 1,
					"patching_rect": [
						20,
						332,
						215,
						22
					],
					"outlettype": [
						""
					],
					"text": "pak 1. 1. 1. 1."
				}
			},
			{
				"box": {
					"id": "p2",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20,
						360,
						230,
						22
					],
					"outlettype": [
						"",
						""
					],
					"text": "ossia.parameter cube/color @type vec4f"
				}
			},
			{
				"box": {
					"id": "c3",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20,
						405,
						320,
						20
					],
					"text": "spin (float, degrees per second)"
				}
			},
			{
				"box": {
					"id": "f3",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20,
						430,
						60,
						22
					],
					"outlettype": [
						"",
						"bang"
					]
				}
			},
			{
				"box": {
					"id": "p3",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20,
						460,
						420,
						22
					],
					"outlettype": [
						"",
						""
					],
					"text": "ossia.parameter cube/spin @type float @min -360. @max 360. @default 45."
				}
			},
			{
				"box": {
					"id": "c4",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20,
						505,
						320,
						20
					],
					"text": "angle (float, sent by Unity as the cube turns)"
				}
			},
			{
				"box": {
					"id": "p4",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20,
						530,
						335,
						22
					],
					"outlettype": [
						"",
						""
					],
					"text": "ossia.parameter cube/angle @type float @min 0. @max 360."
				}
			},
			{
				"box": {
					"id": "ps",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						20,
						560,
						80,
						22
					],
					"outlettype": [
						""
					],
					"text": "prepend set"
				}
			},
			{
				"box": {
					"id": "m4",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"patching_rect": [
						20,
						590,
						120,
						22
					],
					"outlettype": [
						""
					],
					"text": ""
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"source": [
						"lm",
						0
					],
					"destination": [
						"dev",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"ex",
						0
					],
					"destination": [
						"dev",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"dev",
						0
					],
					"destination": [
						"pd",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"f1",
						0
					],
					"destination": [
						"p1",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"f20",
						0
					],
					"destination": [
						"pk",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"f21",
						0
					],
					"destination": [
						"pk",
						1
					]
				}
			},
			{
				"patchline": {
					"source": [
						"f22",
						0
					],
					"destination": [
						"pk",
						2
					]
				}
			},
			{
				"patchline": {
					"source": [
						"f23",
						0
					],
					"destination": [
						"pk",
						3
					]
				}
			},
			{
				"patchline": {
					"source": [
						"pk",
						0
					],
					"destination": [
						"p2",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"f3",
						0
					],
					"destination": [
						"p3",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"p4",
						0
					],
					"destination": [
						"ps",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"ps",
						0
					],
					"destination": [
						"m4",
						0
					]
				}
			}
		]
	}
}