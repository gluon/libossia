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
			260
		],
		"boxes": [
			{
				"box": {
					"id": "obj-1",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20,
						15,
						700,
						20
					],
					"text": "Max peer of the cross bench, run by tests/Unity/cross/cross_run.py with no click: Unity, TouchDesigner and Max.",
					"linecount": 5
				}
			},
			{
				"box": {
					"id": "obj-2",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						20,
						50,
						110,
						22
					],
					"text": "udpreceive 7493",
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-3",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 5,
					"patching_rect": [
						20,
						90,
						520,
						22
					],
					"text": "js cross_peer.js",
					"outlettype": [
						"",
						"",
						"",
						"",
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-4",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						20,
						140,
						130,
						22
					],
					"text": "p role_client",
					"outlettype": [
						""
					],
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
							480
						],
						"boxes": [
							{
								"box": {
									"id": "obj-1",
									"maxclass": "comment",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20,
										15,
										640,
										20
									],
									"text": "Role client: Max follows the Unity device unity_cross with ossia.client and ossia.remote.",
									"linecount": 5
								}
							},
							{
								"box": {
									"id": "obj-2",
									"maxclass": "inlet",
									"numinlets": 0,
									"numoutlets": 1,
									"patching_rect": [
										20,
										45,
										30,
										30
									],
									"text": "inlet",
									"outlettype": [
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-3",
									"maxclass": "outlet",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20,
										420,
										30,
										30
									],
									"text": "outlet"
								}
							},
							{
								"box": {
									"id": "obj-4",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 3,
									"patching_rect": [
										20,
										85,
										160,
										22
									],
									"text": "route client value",
									"outlettype": [
										"",
										"",
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-5",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 1,
									"patching_rect": [
										20,
										130,
										210,
										22
									],
									"text": "ossia.client",
									"outlettype": [
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-6",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 1,
									"patching_rect": [
										20,
										170,
										140,
										22
									],
									"text": "prepend client",
									"outlettype": [
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-7",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 3,
									"patching_rect": [
										250,
										130,
										210,
										22
									],
									"text": "ossia.remote probe/CrossFields/value",
									"outlettype": [
										"",
										"",
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-8",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 1,
									"patching_rect": [
										250,
										170,
										140,
										22
									],
									"text": "prepend remote value",
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
										"obj-2",
										0
									],
									"destination": [
										"obj-4",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-4",
										0
									],
									"destination": [
										"obj-5",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-5",
										0
									],
									"destination": [
										"obj-6",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-6",
										0
									],
									"destination": [
										"obj-3",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-4",
										1
									],
									"destination": [
										"obj-7",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-7",
										1
									],
									"destination": [
										"obj-8",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-8",
										0
									],
									"destination": [
										"obj-3",
										0
									]
								}
							}
						]
					}
				}
			},
			{
				"box": {
					"id": "obj-5",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						170,
						140,
						130,
						22
					],
					"text": "p role_device",
					"outlettype": [
						""
					],
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
							480
						],
						"boxes": [
							{
								"box": {
									"id": "obj-1",
									"maxclass": "comment",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20,
										15,
										640,
										20
									],
									"text": "Role device: Max is the device max_cross (OSC 9880, HTTP and WebSocket 9881) that a Unity client follows.",
									"linecount": 5
								}
							},
							{
								"box": {
									"id": "obj-2",
									"maxclass": "inlet",
									"numinlets": 0,
									"numoutlets": 1,
									"patching_rect": [
										20,
										45,
										30,
										30
									],
									"text": "inlet",
									"outlettype": [
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-3",
									"maxclass": "outlet",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20,
										420,
										30,
										30
									],
									"text": "outlet"
								}
							},
							{
								"box": {
									"id": "obj-4",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 3,
									"patching_rect": [
										20,
										85,
										160,
										22
									],
									"text": "route device value",
									"outlettype": [
										"",
										"",
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-5",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 1,
									"patching_rect": [
										20,
										130,
										210,
										22
									],
									"text": "ossia.device max_cross",
									"outlettype": [
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-6",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 1,
									"patching_rect": [
										20,
										170,
										140,
										22
									],
									"text": "prepend device",
									"outlettype": [
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-7",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 2,
									"patching_rect": [
										250,
										130,
										210,
										22
									],
									"text": "ossia.parameter probe/value @type float @min 0 @max 100000 @default 0.5",
									"outlettype": [
										"",
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-8",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 1,
									"patching_rect": [
										250,
										170,
										140,
										22
									],
									"text": "prepend param value",
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
										"obj-2",
										0
									],
									"destination": [
										"obj-4",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-4",
										0
									],
									"destination": [
										"obj-5",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-5",
										0
									],
									"destination": [
										"obj-6",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-6",
										0
									],
									"destination": [
										"obj-3",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-4",
										1
									],
									"destination": [
										"obj-7",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-7",
										0
									],
									"destination": [
										"obj-8",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-8",
										0
									],
									"destination": [
										"obj-3",
										0
									]
								}
							}
						]
					}
				}
			},
			{
				"box": {
					"id": "obj-6",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						320,
						140,
						130,
						22
					],
					"text": "p role_observer",
					"outlettype": [
						""
					],
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
							480
						],
						"boxes": [
							{
								"box": {
									"id": "obj-1",
									"maxclass": "comment",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20,
										15,
										640,
										20
									],
									"text": "Role observer: Max follows follow_me, the device of FollowADevice.maxpat, by its address.",
									"linecount": 5
								}
							},
							{
								"box": {
									"id": "obj-2",
									"maxclass": "inlet",
									"numinlets": 0,
									"numoutlets": 1,
									"patching_rect": [
										20,
										45,
										30,
										30
									],
									"text": "inlet",
									"outlettype": [
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-3",
									"maxclass": "outlet",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20,
										420,
										30,
										30
									],
									"text": "outlet"
								}
							},
							{
								"box": {
									"id": "obj-4",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 5,
									"patching_rect": [
										20,
										85,
										280,
										22
									],
									"text": "route client size angle spin",
									"outlettype": [
										"",
										"",
										"",
										"",
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-5",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 1,
									"patching_rect": [
										20,
										130,
										210,
										22
									],
									"text": "ossia.client",
									"outlettype": [
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-6",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 1,
									"patching_rect": [
										20,
										170,
										140,
										22
									],
									"text": "prepend oclient",
									"outlettype": [
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-7",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 3,
									"patching_rect": [
										250,
										130,
										210,
										22
									],
									"text": "ossia.remote cube/size",
									"outlettype": [
										"",
										"",
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-8",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 1,
									"patching_rect": [
										250,
										170,
										140,
										22
									],
									"text": "prepend oremote size",
									"outlettype": [
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-9",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 3,
									"patching_rect": [
										480,
										130,
										210,
										22
									],
									"text": "ossia.remote cube/angle",
									"outlettype": [
										"",
										"",
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-10",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 1,
									"patching_rect": [
										480,
										170,
										140,
										22
									],
									"text": "prepend oremote angle",
									"outlettype": [
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-11",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 3,
									"patching_rect": [
										710,
										130,
										210,
										22
									],
									"text": "ossia.remote cube/spin",
									"outlettype": [
										"",
										"",
										""
									]
								}
							},
							{
								"box": {
									"id": "obj-12",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 1,
									"patching_rect": [
										710,
										170,
										140,
										22
									],
									"text": "prepend oremote spin",
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
										"obj-2",
										0
									],
									"destination": [
										"obj-4",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-4",
										0
									],
									"destination": [
										"obj-5",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-5",
										0
									],
									"destination": [
										"obj-6",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-6",
										0
									],
									"destination": [
										"obj-3",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-4",
										1
									],
									"destination": [
										"obj-7",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-7",
										1
									],
									"destination": [
										"obj-8",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-8",
										0
									],
									"destination": [
										"obj-3",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-4",
										2
									],
									"destination": [
										"obj-9",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-9",
										1
									],
									"destination": [
										"obj-10",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-10",
										0
									],
									"destination": [
										"obj-3",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-4",
										3
									],
									"destination": [
										"obj-11",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-11",
										1
									],
									"destination": [
										"obj-12",
										0
									]
								}
							},
							{
								"patchline": {
									"source": [
										"obj-12",
										0
									],
									"destination": [
										"obj-3",
										0
									]
								}
							}
						]
					}
				}
			},
			{
				"box": {
					"id": "obj-7",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						470,
						140,
						150,
						22
					],
					"text": "udpsend 127.0.0.1 7496"
				}
			},
			{
				"box": {
					"id": "obj-8",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						640,
						140,
						60,
						22
					],
					"text": "deferlow",
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-9",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						640,
						175,
						80,
						22
					],
					"text": "thispatcher",
					"outlettype": [
						"",
						""
					]
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"source": [
						"obj-2",
						0
					],
					"destination": [
						"obj-3",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-3",
						0
					],
					"destination": [
						"obj-4",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-4",
						0
					],
					"destination": [
						"obj-3",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-3",
						1
					],
					"destination": [
						"obj-5",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-5",
						0
					],
					"destination": [
						"obj-3",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-3",
						2
					],
					"destination": [
						"obj-6",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-6",
						0
					],
					"destination": [
						"obj-3",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-3",
						3
					],
					"destination": [
						"obj-7",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-3",
						4
					],
					"destination": [
						"obj-8",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-8",
						0
					],
					"destination": [
						"obj-9",
						0
					]
				}
			}
		]
	}
}