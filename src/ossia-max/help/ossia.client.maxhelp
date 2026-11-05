{
	"patcher" : 	{
		"fileversion" : 1,
		"appversion" : 		{
			"major" : 9,
			"minor" : 0,
			"revision" : 4,
			"architecture" : "x64",
			"modernui" : 1
		}
,
		"classnamespace" : "box",
		"rect" : [ 134.0, 134.0, 968.0, 949.0 ],
		"gridsize" : [ 15.0, 15.0 ],
		"showrootpatcherontab" : 0,
		"showontab" : 0,
		"boxes" : [ 			{
				"box" : 				{
					"annotation" : "none",
					"id" : "obj-3",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patcher" : 					{
						"fileversion" : 1,
						"appversion" : 						{
							"major" : 9,
							"minor" : 0,
							"revision" : 4,
							"architecture" : "x64",
							"modernui" : 1
						}
,
						"classnamespace" : "box",
						"rect" : [ 0.0, 26.0, 968.0, 923.0 ],
						"gridsize" : [ 15.0, 15.0 ],
						"showontab" : 1,
						"boxes" : [  ],
						"lines" : [  ],
						"originid" : "pat-22"
					}
,
					"patching_rect" : [ 424.0, 242.0, 27.0, 22.0 ],
					"saved_object_attributes" : 					{
						"globalpatchername" : ""
					}
,
					"text" : "p ?"
				}

			}
, 			{
				"box" : 				{
					"annotation" : "none",
					"id" : "obj-2",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patcher" : 					{
						"fileversion" : 1,
						"appversion" : 						{
							"major" : 9,
							"minor" : 0,
							"revision" : 4,
							"architecture" : "x64",
							"modernui" : 1
						}
,
						"classnamespace" : "box",
						"rect" : [ 0.0, 26.0, 879.0, 805.0 ],
						"gridsize" : [ 15.0, 15.0 ],
						"showontab" : 0,
						"boxes" : [ 							{
								"box" : 								{
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-17",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 101.0, 597.0, 330.0, 21.0 ],
									"text" : "i.e. does the same thing than that:",
									"textcolor" : [ 0.6, 0.6, 0.6, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"format" : 6,
									"id" : "obj-7",
									"maxclass" : "flonum",
									"numinlets" : 1,
									"numoutlets" : 2,
									"outlettype" : [ "", "bang" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 94.5, 633.0, 50.0, 22.0 ]
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-15",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 3,
									"outlettype" : [ "", "", "" ],
									"patching_rect" : [ 151.5, 633.0, 232.0, 22.0 ],
									"text" : "ossia.remote sub_model{\\,.}*/a_parameter"
								}

							}
, 							{
								"box" : 								{
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-14",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 101.0, 536.0, 330.0, 21.0 ],
									"text" : "use '!' to address all instances (including the original instance)",
									"textcolor" : [ 0.6, 0.6, 0.6, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"format" : 6,
									"id" : "obj-12",
									"maxclass" : "flonum",
									"numinlets" : 1,
									"numoutlets" : 2,
									"outlettype" : [ "", "bang" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 101.0, 565.0, 50.0, 22.0 ]
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-13",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 3,
									"outlettype" : [ "", "", "" ],
									"patching_rect" : [ 158.0, 565.0, 223.0, 22.0 ],
									"text" : "ossia.remote sub_model!/a_parameter"
								}

							}
, 							{
								"box" : 								{
									"format" : 6,
									"id" : "obj-10",
									"maxclass" : "flonum",
									"numinlets" : 1,
									"numoutlets" : 2,
									"outlettype" : [ "", "bang" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 101.0, 492.0, 50.0, 22.0 ]
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-11",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 3,
									"outlettype" : [ "", "", "" ],
									"patching_rect" : [ 158.0, 492.0, 223.0, 22.0 ],
									"text" : "ossia.remote sub_model.1/a_parameter"
								}

							}
, 							{
								"box" : 								{
									"format" : 6,
									"id" : "obj-8",
									"maxclass" : "flonum",
									"numinlets" : 1,
									"numoutlets" : 2,
									"outlettype" : [ "", "bang" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 101.0, 441.0, 50.0, 22.0 ]
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-6",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 3,
									"outlettype" : [ "", "", "" ],
									"patching_rect" : [ 158.0, 441.0, 213.0, 22.0 ],
									"text" : "ossia.remote sub_model/a_parameter"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-9",
									"maxclass" : "newobj",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patcher" : 									{
										"fileversion" : 1,
										"appversion" : 										{
											"major" : 9,
											"minor" : 0,
											"revision" : 4,
											"architecture" : "x64",
											"modernui" : 1
										}
,
										"classnamespace" : "box",
										"rect" : [ 59.0, 103.0, 363.0, 605.0 ],
										"gridsize" : [ 15.0, 15.0 ],
										"boxes" : [ 											{
												"box" : 												{
													"id" : "obj-9",
													"maxclass" : "message",
													"numinlets" : 2,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 163.5, 193.0, 50.0, 22.0 ],
													"text" : "clear all"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-7",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 254.5, 340.0, 93.0, 22.0 ],
													"text" : "prepend set set"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-6",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 260.5, 295.0, 65.0, 22.0 ],
													"text" : "prepend 1"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-5",
													"maxclass" : "newobj",
													"numinlets" : 2,
													"numoutlets" : 2,
													"outlettype" : [ "", "" ],
													"patching_rect" : [ 31.5, 329.5, 100.0, 22.0 ],
													"text" : "zl slice 1"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-2",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 151.5, 394.0, 74.0, 22.0 ],
													"text" : "prepend set"
												}

											}
, 											{
												"box" : 												{
													"comment" : "",
													"id" : "obj-1",
													"index" : 1,
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 56.5, 36.0, 30.0, 30.0 ]
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-31",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 2,
													"outlettype" : [ "", "bang" ],
													"patching_rect" : [ 50.0, 239.0, 30.0, 22.0 ],
													"text" : "t l b"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-28",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 44.5, 394.0, 74.0, 22.0 ],
													"text" : "prepend set"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-27",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 181.5, 295.0, 65.0, 22.0 ],
													"text" : "prepend 0"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-26",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 151.5, 340.0, 93.0, 22.0 ],
													"text" : "prepend set set"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-25",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 3,
													"outlettype" : [ "", "bang", "int" ],
													"patching_rect" : [ 147.5, 96.0, 67.0, 22.0 ],
													"text" : "t s b -1"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-24",
													"maxclass" : "newobj",
													"numinlets" : 5,
													"numoutlets" : 4,
													"outlettype" : [ "int", "", "", "int" ],
													"patching_rect" : [ 196.5, 239.0, 61.0, 22.0 ],
													"text" : "counter"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-15",
													"maxclass" : "newobj",
													"numinlets" : 2,
													"numoutlets" : 2,
													"outlettype" : [ "", "" ],
													"patching_rect" : [ 50.0, 154.0, 103.0, 22.0 ],
													"text" : "route namespace"
												}

											}
, 											{
												"box" : 												{
													"comment" : "",
													"id" : "obj-34",
													"index" : 2,
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 147.5, 36.0, 30.0, 30.0 ]
												}

											}
, 											{
												"box" : 												{
													"comment" : "",
													"id" : "obj-35",
													"index" : 1,
													"maxclass" : "outlet",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 37.5, 502.0, 30.0, 30.0 ]
												}

											}
 ],
										"lines" : [ 											{
												"patchline" : 												{
													"destination" : [ "obj-15", 0 ],
													"source" : [ "obj-1", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-31", 0 ],
													"source" : [ "obj-15", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-35", 0 ],
													"source" : [ "obj-2", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-27", 0 ],
													"order" : 1,
													"source" : [ "obj-24", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-6", 0 ],
													"order" : 0,
													"source" : [ "obj-24", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-24", 3 ],
													"source" : [ "obj-25", 2 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-9", 0 ],
													"source" : [ "obj-25", 1 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-28", 0 ],
													"source" : [ "obj-26", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-26", 0 ],
													"source" : [ "obj-27", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-35", 0 ],
													"source" : [ "obj-28", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-24", 0 ],
													"source" : [ "obj-31", 1 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-5", 0 ],
													"source" : [ "obj-31", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-25", 0 ],
													"source" : [ "obj-34", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-2", 0 ],
													"source" : [ "obj-5", 1 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-28", 0 ],
													"source" : [ "obj-5", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-7", 0 ],
													"source" : [ "obj-6", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-2", 0 ],
													"source" : [ "obj-7", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-35", 0 ],
													"source" : [ "obj-9", 0 ]
												}

											}
 ],
										"originid" : "pat-26"
									}
,
									"patching_rect" : [ 485.0, 284.0, 286.0, 22.0 ],
									"saved_object_attributes" : 									{
										"globalpatchername" : ""
									}
,
									"text" : "p get_namespace"
								}

							}
, 							{
								"box" : 								{
									"coldef" : [ [ 1, 66, 1, 0.0, 0.0, 0.0, 1.0, 1, 0.0, 0.0, 0.0, 1.0, -1, -1, -1 ] ],
									"cols" : 2,
									"colwidth" : 240,
									"fontface" : 0,
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"hscroll" : 0,
									"id" : "obj-16",
									"maxclass" : "jit.cellblock",
									"numinlets" : 2,
									"numoutlets" : 4,
									"outlettype" : [ "list", "", "", "" ],
									"patching_rect" : [ 485.0, 325.0, 324.0, 288.0 ]
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-5",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 485.0, 258.0, 170.0, 22.0 ],
									"text" : "ossia.device model_instances"
								}

							}
, 							{
								"box" : 								{
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-3",
									"linecount" : 2,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 121.0, 325.0, 207.0, 35.0 ],
									"text" : "Another submodel named sub_model (see inside ossia.model argument)",
									"textcolor" : [ 0.6, 0.6, 0.6, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-4",
									"linecount" : 4,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 4.5, 59.0, 440.0, 64.0 ],
									"text" : "If 2 or more models have the same name at the same level (like in this example, with two submodels being named identically), they will be registered as the next available instances (instances are appended to the node name, after a dot separator)",
									"textcolor" : [ 0.6, 0.6, 0.6, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"bubble" : 1,
									"bubbletextmargin" : 10,
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-39",
									"linecount" : 3,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 591.0, 188.0, 196.0, 64.0 ],
									"text" : "Click 'namespace' message to get all this patcher's nodes addresse",
									"textcolor" : [ 0.3, 0.3, 0.3, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-18",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 485.0, 220.0, 73.0, 22.0 ],
									"text" : "namespace"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-2",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 0,
									"patcher" : 									{
										"fileversion" : 1,
										"appversion" : 										{
											"major" : 9,
											"minor" : 0,
											"revision" : 4,
											"architecture" : "x64",
											"modernui" : 1
										}
,
										"classnamespace" : "box",
										"rect" : [ 1236.0, 326.0, 310.0, 264.0 ],
										"gridsize" : [ 15.0, 15.0 ],
										"boxes" : [ 											{
												"box" : 												{
													"format" : 6,
													"id" : "obj-5",
													"maxclass" : "flonum",
													"numinlets" : 1,
													"numoutlets" : 2,
													"outlettype" : [ "", "bang" ],
													"parameter_enable" : 0,
													"patching_rect" : [ 36.0, 158.0, 50.0, 22.0 ]
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-3",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 2,
													"outlettype" : [ "", "" ],
													"patching_rect" : [ 36.0, 188.0, 202.0, 22.0 ],
													"text" : "ossia.parameter another_parameter"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-2",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 2,
													"outlettype" : [ "", "" ],
													"patching_rect" : [ 36.0, 129.0, 168.0, 22.0 ],
													"text" : "ossia.parameter a_parameter"
												}

											}
, 											{
												"box" : 												{
													"color" : [ 0.989256, 0.417462, 0.031845, 1.0 ],
													"id" : "obj-1",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 36.0, 43.0, 137.0, 22.0 ],
													"text" : "ossia.model sub_model"
												}

											}
 ],
										"lines" : [ 											{
												"patchline" : 												{
													"destination" : [ "obj-5", 0 ],
													"source" : [ "obj-2", 0 ]
												}

											}
 ],
										"originid" : "pat-28"
									}
,
									"patching_rect" : [ 121.0, 294.0, 173.0, 22.0 ],
									"saved_object_attributes" : 									{
										"globalpatchername" : ""
									}
,
									"text" : "p sub_model_second instance"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-1",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 0,
									"patcher" : 									{
										"fileversion" : 1,
										"appversion" : 										{
											"major" : 9,
											"minor" : 0,
											"revision" : 4,
											"architecture" : "x64",
											"modernui" : 1
										}
,
										"classnamespace" : "box",
										"rect" : [ 1247.0, 692.0, 303.0, 245.0 ],
										"gridsize" : [ 15.0, 15.0 ],
										"boxes" : [ 											{
												"box" : 												{
													"format" : 6,
													"id" : "obj-5",
													"maxclass" : "flonum",
													"numinlets" : 1,
													"numoutlets" : 2,
													"outlettype" : [ "", "bang" ],
													"parameter_enable" : 0,
													"patching_rect" : [ 36.0, 143.0, 50.0, 22.0 ]
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-3",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 2,
													"outlettype" : [ "", "" ],
													"patching_rect" : [ 36.0, 179.0, 202.0, 22.0 ],
													"text" : "ossia.parameter another_parameter"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-2",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 2,
													"outlettype" : [ "", "" ],
													"patching_rect" : [ 36.0, 111.0, 168.0, 22.0 ],
													"text" : "ossia.parameter a_parameter"
												}

											}
, 											{
												"box" : 												{
													"color" : [ 0.989256, 0.417462, 0.031845, 1.0 ],
													"id" : "obj-1",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 36.0, 34.0, 137.0, 22.0 ],
													"text" : "ossia.model sub_model"
												}

											}
 ],
										"lines" : [ 											{
												"patchline" : 												{
													"destination" : [ "obj-5", 0 ],
													"source" : [ "obj-2", 0 ]
												}

											}
 ],
										"originid" : "pat-30"
									}
,
									"patching_rect" : [ 121.0, 258.0, 79.0, 22.0 ],
									"saved_object_attributes" : 									{
										"globalpatchername" : ""
									}
,
									"text" : "p sub_model"
								}

							}
, 							{
								"box" : 								{
									"border" : 0,
									"filename" : "helpdetails.js",
									"id" : "obj-19",
									"ignoreclick" : 1,
									"jsarguments" : [ "ossia.model" ],
									"maxclass" : "jsui",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 8.0, 7.0, 649.5, 50.0 ]
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"destination" : [ "obj-11", 0 ],
									"source" : [ "obj-10", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-10", 0 ],
									"source" : [ "obj-11", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-13", 0 ],
									"source" : [ "obj-12", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-12", 0 ],
									"source" : [ "obj-13", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-7", 0 ],
									"source" : [ "obj-15", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-5", 0 ],
									"order" : 1,
									"source" : [ "obj-18", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-9", 1 ],
									"midpoints" : [ 494.5, 251.5, 761.5, 251.5 ],
									"order" : 0,
									"source" : [ "obj-18", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-9", 0 ],
									"source" : [ "obj-5", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-8", 0 ],
									"source" : [ "obj-6", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-15", 0 ],
									"source" : [ "obj-7", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-6", 0 ],
									"source" : [ "obj-8", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-16", 0 ],
									"midpoints" : [ 494.5, 320.0, 494.5, 320.0 ],
									"source" : [ "obj-9", 0 ]
								}

							}
 ],
						"originid" : "pat-24",
						"styles" : [ 							{
								"name" : "Jamoma_highlighted_orange",
								"default" : 								{
									"accentcolor" : [ 1.0, 0.5, 0.0, 1.0 ]
								}
,
								"parentstyle" : "",
								"multi" : 0
							}
 ]
					}
,
					"patching_rect" : [ 253.0, 243.0, 132.0, 22.0 ],
					"saved_object_attributes" : 					{
						"globalpatchername" : ""
					}
,
					"text" : "p \"Accessing devices\""
				}

			}
, 			{
				"box" : 				{
					"annotation" : "none",
					"id" : "obj-1",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patcher" : 					{
						"fileversion" : 1,
						"appversion" : 						{
							"major" : 9,
							"minor" : 0,
							"revision" : 4,
							"architecture" : "x64",
							"modernui" : 1
						}
,
						"classnamespace" : "box",
						"rect" : [ 134.0, 160.0, 968.0, 923.0 ],
						"gridsize" : [ 15.0, 15.0 ],
						"showontab" : 1,
						"boxes" : [ 							{
								"box" : 								{
									"bubble" : 1,
									"bubbletextmargin" : 10,
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-51",
									"linecount" : 2,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 447.0, 564.0, 145.0, 49.0 ],
									"text" : "Get the distant device's namespace"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-45",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 352.0, 577.5, 72.0, 22.0 ],
									"text" : "namespace"
								}

							}
, 							{
								"box" : 								{
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-49",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 88.0, 351.5, 328.0, 21.0 ],
									"text" : "In other cases, connections can be achieved these ways also:"
								}

							}
, 							{
								"box" : 								{
									"bubble" : 1,
									"bubbleside" : 0,
									"bubbletextmargin" : 10,
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-48",
									"linecount" : 2,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 393.0, 256.5, 226.0, 64.0 ],
									"text" : "ossia.view allows to bind all remotes in this patcher to the remote node"
								}

							}
, 							{
								"box" : 								{
									"bubble" : 1,
									"bubbletextmargin" : 10,
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-47",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 197.0, 572.5, 120.0, 35.0 ],
									"text" : "manage presets"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-46",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patcher" : 									{
										"fileversion" : 1,
										"appversion" : 										{
											"major" : 9,
											"minor" : 0,
											"revision" : 4,
											"architecture" : "x64",
											"modernui" : 1
										}
,
										"classnamespace" : "box",
										"rect" : [ 59.0, 103.0, 364.0, 546.0 ],
										"gridsize" : [ 15.0, 15.0 ],
										"boxes" : [ 											{
												"box" : 												{
													"id" : "obj-8",
													"linecount" : 2,
													"maxclass" : "comment",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 42.0, 444.0, 215.0, 33.0 ],
													"text" : "values for parameters with recall_safe attribute set to 1 will not be recalled"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-5",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 2,
													"outlettype" : [ "", "" ],
													"patching_rect" : [ 38.0, 479.0, 245.0, 22.0 ],
													"text" : "ossia.parameter no_recall @recall_safe 1"
												}

											}
, 											{
												"box" : 												{
													"bubble" : 1,
													"bubbleside" : 2,
													"bubbletextmargin" : 10,
													"fontname" : "Lato Regular",
													"fontsize" : 12.0,
													"id" : "obj-49",
													"maxclass" : "comment",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 160.0, 100.0, 50.0, 50.0 ],
													"text" : "recall",
													"textcolor" : [ 0.3, 0.3, 0.3, 1.0 ]
												}

											}
, 											{
												"box" : 												{
													"bubble" : 1,
													"bubbleside" : 2,
													"bubbletextmargin" : 10,
													"fontname" : "Lato Regular",
													"fontsize" : 12.0,
													"id" : "obj-48",
													"maxclass" : "comment",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 59.0, 100.0, 46.0, 50.0 ],
													"text" : "save",
													"textcolor" : [ 0.3, 0.3, 0.3, 1.0 ]
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-47",
													"maxclass" : "comment",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 114.0, 107.5, 39.0, 20.0 ],
													"text" : "and"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-44",
													"maxclass" : "message",
													"numinlets" : 2,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 71.0, 189.5, 72.0, 22.0 ],
													"text" : "~/test2.txt"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-43",
													"maxclass" : "message",
													"numinlets" : 2,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 147.0, 155.5, 72.0, 22.0 ],
													"text" : "~/test1.txt"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-40",
													"linecount" : 2,
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 147.0, 266.5, 73.0, 35.0 ],
													"text" : "prepend preset load"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-41",
													"maxclass" : "message",
													"numinlets" : 2,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 174.0, 189.5, 72.0, 22.0 ],
													"text" : "~/test2.txt"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-42",
													"linecount" : 2,
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 2,
													"outlettype" : [ "", "int" ],
													"patching_rect" : [ 147.0, 225.0, 82.0, 35.0 ],
													"text" : "conformpath max boot"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-39",
													"linecount" : 2,
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 50.0, 266.5, 73.0, 35.0 ],
													"text" : "prepend preset save"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-36",
													"maxclass" : "message",
													"numinlets" : 2,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 50.0, 155.5, 72.0, 22.0 ],
													"text" : "~/test1.txt"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-28",
													"linecount" : 2,
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 2,
													"outlettype" : [ "", "int" ],
													"patching_rect" : [ 50.0, 225.0, 82.0, 35.0 ],
													"text" : "conformpath max boot"
												}

											}
, 											{
												"box" : 												{
													"comment" : "",
													"id" : "obj-45",
													"index" : 1,
													"maxclass" : "outlet",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 92.5, 361.5, 30.0, 30.0 ]
												}

											}
 ],
										"lines" : [ 											{
												"patchline" : 												{
													"destination" : [ "obj-39", 0 ],
													"source" : [ "obj-28", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-28", 0 ],
													"source" : [ "obj-36", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-45", 0 ],
													"source" : [ "obj-39", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-45", 0 ],
													"source" : [ "obj-40", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-42", 0 ],
													"source" : [ "obj-41", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-40", 0 ],
													"source" : [ "obj-42", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-42", 0 ],
													"source" : [ "obj-43", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-28", 0 ],
													"source" : [ "obj-44", 0 ]
												}

											}
 ],
										"originid" : "pat-34"
									}
,
									"patching_rect" : [ 126.0, 579.0, 60.0, 22.0 ],
									"saved_object_attributes" : 									{
										"globalpatchername" : ""
									}
,
									"text" : "p presets"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-30",
									"linecount" : 2,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 720.0, 100.5, 187.0, 33.0 ],
									"text" : "the device name can be prepended to the address, too"
								}

							}
, 							{
								"box" : 								{
									"bgcolor" : [ 0.269595890603871, 0.408853959740371, 0.595943257030903, 1.0 ],
									"bgoncolor" : [ 0.93, 0.93, 0.93, 1.0 ],
									"fontface" : 1,
									"fontname" : "Helvetica Neue Bold",
									"id" : "obj-33",
									"ignoreclick" : 1,
									"legacytextcolor" : 1,
									"maxclass" : "textbutton",
									"numinlets" : 1,
									"numoutlets" : 3,
									"outlettype" : [ "", "", "int" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 648.0, 216.0, 24.0, 24.0 ],
									"rounded" : 180.0,
									"saved_attribute_attributes" : 									{
										"bgcolor" : 										{
											"expression" : "themecolor.live_selection"
										}
,
										"textcolor" : 										{
											"expression" : "themecolor.live_control_text_selection"
										}

									}
,
									"text" : "5",
									"textcolor" : [ 0.85, 0.85, 0.85, 1.0 ],
									"textoncolor" : [ 1.0, 1.0, 1.0, 1.0 ],
									"textovercolor" : [ 1.0, 1.0, 1.0, 1.0 ],
									"usebgoncolor" : 1,
									"usetextovercolor" : 1
								}

							}
, 							{
								"box" : 								{
									"bubble" : 1,
									"bubbleside" : 2,
									"bubbletextmargin" : 10,
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-31",
									"linecount" : 2,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 677.0, 205.0, 207.0, 64.0 ],
									"text" : "ossia.remotes now automatically bind to the distant ossia.device"
								}

							}
, 							{
								"box" : 								{
									"bubble" : 1,
									"bubbletextmargin" : 10,
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-29",
									"linecount" : 2,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 682.0, 380.5, 221.0, 49.0 ],
									"text" : "get list of zeroconf-compliant devices available on the network"
								}

							}
, 							{
								"box" : 								{
									"bubble" : 1,
									"bubbletextmargin" : 10,
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-25",
									"linecount" : 2,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 690.0, 431.5, 207.0, 49.0 ],
									"text" : "get info in the Max window on how to use the connect message"
								}

							}
, 							{
								"box" : 								{
									"bubble" : 1,
									"bubbletextmargin" : 10,
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-24",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 176.0, 507.0, 247.0, 35.0 ],
									"text" : "disconnect from the connected device"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-15",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 626.0, 445.0, 53.0, 22.0 ],
									"text" : "connect"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-23",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 355.0, 699.0, 34.0, 22.0 ],
									"text" : "print"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-20",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 604.0, 392.0, 69.0, 22.0 ],
									"text" : "get devices"
								}

							}
, 							{
								"box" : 								{
									"bubble" : 1,
									"bubbletextmargin" : 10,
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-38",
									"linecount" : 2,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 702.0, 507.0, 167.0, 49.0 ],
									"text" : "Report changes in the distant namespace"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-37",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 649.0, 520.5, 47.0, 22.0 ],
									"text" : "update"
								}

							}
, 							{
								"box" : 								{
									"format" : 6,
									"id" : "obj-34",
									"maxclass" : "flonum",
									"numinlets" : 1,
									"numoutlets" : 2,
									"outlettype" : [ "", "bang" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 804.0, 321.0, 50.0, 22.0 ]
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-35",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 3,
									"outlettype" : [ "", "", "" ],
									"patching_rect" : [ 804.0, 289.0, 100.0, 22.0 ],
									"text" : "ossia.remote bar"
								}

							}
, 							{
								"box" : 								{
									"bgcolor" : [ 0.269595890603871, 0.408853959740371, 0.595943257030903, 1.0 ],
									"bgoncolor" : [ 0.93, 0.93, 0.93, 1.0 ],
									"fontface" : 1,
									"fontname" : "Helvetica Neue Bold",
									"id" : "obj-32",
									"ignoreclick" : 1,
									"legacytextcolor" : 1,
									"maxclass" : "textbutton",
									"numinlets" : 1,
									"numoutlets" : 3,
									"outlettype" : [ "", "", "int" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 393.0, 225.0, 24.0, 24.0 ],
									"rounded" : 180.0,
									"saved_attribute_attributes" : 									{
										"bgcolor" : 										{
											"expression" : "themecolor.live_selection"
										}
,
										"textcolor" : 										{
											"expression" : "themecolor.live_control_text_selection"
										}

									}
,
									"text" : "4",
									"textcolor" : [ 0.85, 0.85, 0.85, 1.0 ],
									"textoncolor" : [ 1.0, 1.0, 1.0, 1.0 ],
									"textovercolor" : [ 1.0, 1.0, 1.0, 1.0 ],
									"usebgoncolor" : 1,
									"usetextovercolor" : 1
								}

							}
, 							{
								"box" : 								{
									"format" : 6,
									"id" : "obj-26",
									"maxclass" : "flonum",
									"numinlets" : 1,
									"numoutlets" : 2,
									"outlettype" : [ "", "bang" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 664.0, 106.0, 50.0, 22.0 ]
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-27",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 3,
									"outlettype" : [ "", "", "" ],
									"patching_rect" : [ 664.0, 61.0, 281.0, 22.0 ],
									"text" : "ossia.remote distant_device:/my_distant_model/foo"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-1",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 419.0, 226.0, 167.0, 22.0 ],
									"text" : "ossia.view my_distant_model"
								}

							}
, 							{
								"box" : 								{
									"format" : 6,
									"id" : "obj-22",
									"maxclass" : "flonum",
									"numinlets" : 1,
									"numoutlets" : 2,
									"outlettype" : [ "", "bang" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 677.0, 316.0, 50.0, 22.0 ]
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-17",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 3,
									"outlettype" : [ "", "", "" ],
									"patching_rect" : [ 677.0, 289.0, 99.0, 22.0 ],
									"text" : "ossia.remote foo"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-13",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 88.0, 513.5, 68.0, 22.0 ],
									"text" : "disconnect"
								}

							}
, 							{
								"box" : 								{
									"bubble" : 1,
									"bubbletextmargin" : 10,
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-12",
									"linecount" : 3,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 304.0, 428.5, 247.0, 64.0 ],
									"text" : "oscquery & Minuit enabled devices can be connected using connect message followed by the name of the device"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-10",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 86.0, 449.5, 213.0, 22.0 ],
									"text" : "connect oscquery ws://127.0.0.1:9876"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-8",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 82.0, 395.5, 138.0, 22.0 ],
									"text" : "connect distant_device"
								}

							}
, 							{
								"box" : 								{
									"bubble" : 1,
									"bubbletextmargin" : 10,
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-6",
									"linecount" : 2,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 230.0, 374.5, 350.0, 49.0 ],
									"text" : "oscquery & Minuit enabled devices can be connected using connect message followed by the name of the device"
								}

							}
, 							{
								"box" : 								{
									"bgcolor" : [ 0.269595890603871, 0.408853959740371, 0.595943257030903, 1.0 ],
									"bgoncolor" : [ 0.93, 0.93, 0.93, 1.0 ],
									"fontface" : 1,
									"fontname" : "Helvetica Neue Bold",
									"id" : "obj-4",
									"ignoreclick" : 1,
									"legacytextcolor" : 1,
									"maxclass" : "textbutton",
									"numinlets" : 1,
									"numoutlets" : 3,
									"outlettype" : [ "", "", "int" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 28.5, 287.0, 24.0, 24.0 ],
									"rounded" : 180.0,
									"saved_attribute_attributes" : 									{
										"bgcolor" : 										{
											"expression" : "themecolor.live_selection"
										}
,
										"textcolor" : 										{
											"expression" : "themecolor.live_control_text_selection"
										}

									}
,
									"text" : "3",
									"textcolor" : [ 0.85, 0.85, 0.85, 1.0 ],
									"textoncolor" : [ 1.0, 1.0, 1.0, 1.0 ],
									"textovercolor" : [ 1.0, 1.0, 1.0, 1.0 ],
									"usebgoncolor" : 1,
									"usetextovercolor" : 1
								}

							}
, 							{
								"box" : 								{
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-3",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 164.5, 835.0, 132.0, 21.0 ],
									"text" : "Discovered parameters"
								}

							}
, 							{
								"box" : 								{
									"bgcolor" : [ 0.269595890603871, 0.408853959740371, 0.595943257030903, 1.0 ],
									"bgoncolor" : [ 0.93, 0.93, 0.93, 1.0 ],
									"fontface" : 1,
									"fontname" : "Helvetica Neue Bold",
									"id" : "obj-21",
									"ignoreclick" : 1,
									"legacytextcolor" : 1,
									"maxclass" : "textbutton",
									"numinlets" : 1,
									"numoutlets" : 3,
									"outlettype" : [ "", "", "int" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 22.5, 190.5, 24.0, 24.0 ],
									"rounded" : 180.0,
									"saved_attribute_attributes" : 									{
										"bgcolor" : 										{
											"expression" : "themecolor.live_selection"
										}
,
										"textcolor" : 										{
											"expression" : "themecolor.live_control_text_selection"
										}

									}
,
									"text" : "1",
									"textcolor" : [ 0.85, 0.85, 0.85, 1.0 ],
									"textoncolor" : [ 1.0, 1.0, 1.0, 1.0 ],
									"textovercolor" : [ 1.0, 1.0, 1.0, 1.0 ],
									"usebgoncolor" : 1,
									"usetextovercolor" : 1
								}

							}
, 							{
								"box" : 								{
									"bubble" : 1,
									"bubbletextmargin" : 10,
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-18",
									"linecount" : 4,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 186.0, 162.5, 136.0, 78.0 ],
									"text" : "Open distant ossia.device and expose it through oscquery protocole"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-11",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 62.0, 224.5, 53.0, 22.0 ],
									"text" : "pcontrol"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-7",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 62.0, 190.5, 113.0, 22.0 ],
									"text" : "load distant_device"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-5",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 60.0, 289.0, 105.0, 22.0 ],
									"text" : "connect oscquery"
								}

							}
, 							{
								"box" : 								{
									"bubble" : 1,
									"bubbletextmargin" : 10,
									"fontname" : "Lato Regular",
									"fontsize" : 12.0,
									"id" : "obj-14",
									"linecount" : 2,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 172.0, 275.5, 197.0, 49.0 ],
									"text" : "Connect to the distant device (here with default ports)"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-9",
									"maxclass" : "newobj",
									"numinlets" : 2,
									"numoutlets" : 2,
									"outlettype" : [ "", "" ],
									"patcher" : 									{
										"fileversion" : 1,
										"appversion" : 										{
											"major" : 9,
											"minor" : 0,
											"revision" : 4,
											"architecture" : "x64",
											"modernui" : 1
										}
,
										"classnamespace" : "box",
										"rect" : [ 376.0, 303.0, 363.0, 605.0 ],
										"gridsize" : [ 15.0, 15.0 ],
										"boxes" : [ 											{
												"box" : 												{
													"id" : "obj-8",
													"linecount" : 2,
													"maxclass" : "newobj",
													"numinlets" : 2,
													"numoutlets" : 2,
													"outlettype" : [ "", "" ],
													"patching_rect" : [ 44.5, 144.0, 75.0, 35.0 ],
													"text" : "route namespace"
												}

											}
, 											{
												"box" : 												{
													"comment" : "",
													"id" : "obj-4",
													"index" : 2,
													"maxclass" : "outlet",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 101.5, 185.0, 30.0, 30.0 ]
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-3",
													"maxclass" : "newobj",
													"numinlets" : 2,
													"numoutlets" : 2,
													"outlettype" : [ "", "" ],
													"patching_rect" : [ 12.5, 120.0, 75.0, 22.0 ],
													"text" : "route create"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-9",
													"maxclass" : "message",
													"numinlets" : 2,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 163.5, 193.0, 50.0, 22.0 ],
													"text" : "clear all"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-7",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 254.5, 340.0, 93.0, 22.0 ],
													"text" : "prepend set set"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-6",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 260.5, 295.0, 65.0, 22.0 ],
													"text" : "prepend 1"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-5",
													"maxclass" : "newobj",
													"numinlets" : 2,
													"numoutlets" : 2,
													"outlettype" : [ "", "" ],
													"patching_rect" : [ 31.5, 329.5, 100.0, 22.0 ],
													"text" : "zl slice 1"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-2",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 151.5, 394.0, 74.0, 22.0 ],
													"text" : "prepend set"
												}

											}
, 											{
												"box" : 												{
													"comment" : "",
													"id" : "obj-1",
													"index" : 1,
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 12.5, 36.0, 30.0, 30.0 ]
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-31",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 2,
													"outlettype" : [ "", "bang" ],
													"patching_rect" : [ 50.0, 239.0, 30.0, 22.0 ],
													"text" : "t l b"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-28",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 44.5, 394.0, 74.0, 22.0 ],
													"text" : "prepend set"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-27",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 181.5, 295.0, 65.0, 22.0 ],
													"text" : "prepend 0"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-26",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 151.5, 340.0, 93.0, 22.0 ],
													"text" : "prepend set set"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-25",
													"maxclass" : "newobj",
													"numinlets" : 1,
													"numoutlets" : 3,
													"outlettype" : [ "", "bang", "int" ],
													"patching_rect" : [ 147.5, 96.0, 67.0, 22.0 ],
													"text" : "t s b -1"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-24",
													"maxclass" : "newobj",
													"numinlets" : 5,
													"numoutlets" : 4,
													"outlettype" : [ "int", "", "", "int" ],
													"patching_rect" : [ 196.5, 239.0, 61.0, 22.0 ],
													"text" : "counter"
												}

											}
, 											{
												"box" : 												{
													"id" : "obj-15",
													"maxclass" : "newobj",
													"numinlets" : 2,
													"numoutlets" : 2,
													"outlettype" : [ "", "" ],
													"patching_rect" : [ 12.5, 84.0, 96.0, 22.0 ],
													"text" : "route parameter"
												}

											}
, 											{
												"box" : 												{
													"comment" : "",
													"id" : "obj-34",
													"index" : 2,
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"outlettype" : [ "" ],
													"patching_rect" : [ 147.5, 36.0, 30.0, 30.0 ]
												}

											}
, 											{
												"box" : 												{
													"comment" : "",
													"id" : "obj-35",
													"index" : 1,
													"maxclass" : "outlet",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 37.5, 502.0, 30.0, 30.0 ]
												}

											}
 ],
										"lines" : [ 											{
												"patchline" : 												{
													"destination" : [ "obj-15", 0 ],
													"source" : [ "obj-1", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-3", 0 ],
													"source" : [ "obj-15", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-8", 0 ],
													"source" : [ "obj-15", 1 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-35", 0 ],
													"source" : [ "obj-2", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-27", 0 ],
													"order" : 1,
													"source" : [ "obj-24", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-6", 0 ],
													"order" : 0,
													"source" : [ "obj-24", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-24", 3 ],
													"source" : [ "obj-25", 2 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-9", 0 ],
													"source" : [ "obj-25", 1 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-28", 0 ],
													"source" : [ "obj-26", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-26", 0 ],
													"source" : [ "obj-27", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-35", 0 ],
													"source" : [ "obj-28", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-31", 0 ],
													"source" : [ "obj-3", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-24", 0 ],
													"source" : [ "obj-31", 1 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-5", 0 ],
													"source" : [ "obj-31", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-25", 0 ],
													"source" : [ "obj-34", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-2", 0 ],
													"source" : [ "obj-5", 1 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-28", 0 ],
													"source" : [ "obj-5", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-7", 0 ],
													"source" : [ "obj-6", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-2", 0 ],
													"source" : [ "obj-7", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-31", 0 ],
													"source" : [ "obj-8", 0 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-4", 0 ],
													"source" : [ "obj-8", 1 ]
												}

											}
, 											{
												"patchline" : 												{
													"destination" : [ "obj-35", 0 ],
													"source" : [ "obj-9", 0 ]
												}

											}
 ],
										"originid" : "pat-36"
									}
,
									"patching_rect" : [ 60.0, 703.0, 286.0, 22.0 ],
									"saved_object_attributes" : 									{
										"globalpatchername" : ""
									}
,
									"text" : "p get_namespace"
								}

							}
, 							{
								"box" : 								{
									"coldef" : [ [ 1, 66, 1, 0.0, 0.0, 0.0, 1.0, 1, 0.0, 0.0, 0.0, 1.0, -1, -1, -1 ] ],
									"cols" : 2,
									"colwidth" : 240,
									"fontface" : 0,
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"hscroll" : 0,
									"id" : "obj-16",
									"maxclass" : "jit.cellblock",
									"numinlets" : 2,
									"numoutlets" : 4,
									"outlettype" : [ "list", "", "", "" ],
									"patching_rect" : [ 60.0, 740.0, 329.0, 93.0 ],
									"rows" : 50
								}

							}
, 							{
								"box" : 								{
									"color" : [ 0.0, 0.933333333333333, 1.0, 1.0 ],
									"id" : "obj-2",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 60.0, 658.0, 70.0, 22.0 ],
									"saved_attribute_attributes" : 									{
										"color" : 										{
											"expression" : "themecolor.live_threshold_line_color"
										}

									}
,
									"saved_newobj_attribute_attributes" : 									{
										"color" : 										{
											"expression" : "themecolor.live_threshold_line_color"
										}

									}
,
									"text" : "ossia.client"
								}

							}
, 							{
								"box" : 								{
									"border" : 0,
									"filename" : "helpdetails.js",
									"id" : "obj-19",
									"ignoreclick" : 1,
									"jsarguments" : [ "ossia.client" ],
									"maxclass" : "jsui",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 8.0, 7.0, 561.5, 143.0 ]
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"destination" : [ "obj-2", 0 ],
									"midpoints" : [ 95.5, 481.75, 69.5, 481.75 ],
									"source" : [ "obj-10", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-2", 0 ],
									"midpoints" : [ 97.5, 643.75, 69.5, 643.75 ],
									"source" : [ "obj-13", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-2", 0 ],
									"midpoints" : [ 635.5, 640.0, 69.5, 640.0 ],
									"source" : [ "obj-15", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-22", 0 ],
									"source" : [ "obj-17", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-9", 0 ],
									"source" : [ "obj-2", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-2", 0 ],
									"midpoints" : [ 613.5, 642.5, 69.5, 642.5 ],
									"source" : [ "obj-20", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-17", 0 ],
									"midpoints" : [ 686.5, 346.0, 664.0, 346.0, 664.0, 283.0, 686.5, 283.0 ],
									"source" : [ "obj-22", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-27", 0 ],
									"midpoints" : [ 673.5, 135.0, 651.0, 135.0, 651.0, 43.0, 673.5, 43.0 ],
									"source" : [ "obj-26", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-26", 0 ],
									"source" : [ "obj-27", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-35", 0 ],
									"midpoints" : [ 813.5, 346.0, 791.0, 346.0, 791.0, 283.0, 813.5, 283.0 ],
									"source" : [ "obj-34", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-34", 0 ],
									"source" : [ "obj-35", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-2", 0 ],
									"midpoints" : [ 658.5, 642.0, 69.5, 642.0 ],
									"order" : 1,
									"source" : [ "obj-37", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-9", 1 ],
									"midpoints" : [ 658.5, 642.5, 336.5, 642.5 ],
									"order" : 0,
									"source" : [ "obj-37", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-2", 0 ],
									"midpoints" : [ 361.5, 641.75, 69.5, 641.75 ],
									"order" : 1,
									"source" : [ "obj-45", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-9", 1 ],
									"midpoints" : [ 361.5, 642.25, 336.5, 642.25 ],
									"order" : 0,
									"source" : [ "obj-45", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-2", 0 ],
									"midpoints" : [ 135.5, 640.0, 69.5, 640.0 ],
									"source" : [ "obj-46", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-2", 0 ],
									"midpoints" : [ 69.5, 453.5, 69.5, 453.5 ],
									"order" : 1,
									"source" : [ "obj-5", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-9", 1 ],
									"midpoints" : [ 69.5, 644.0, 336.5, 644.0 ],
									"order" : 0,
									"source" : [ "obj-5", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-11", 0 ],
									"source" : [ "obj-7", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-2", 0 ],
									"midpoints" : [ 91.5, 424.9921875, 69.5, 424.9921875 ],
									"source" : [ "obj-8", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-16", 0 ],
									"midpoints" : [ 69.5, 731.0, 69.5, 731.0 ],
									"source" : [ "obj-9", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-23", 0 ],
									"source" : [ "obj-9", 1 ]
								}

							}
 ],
						"originid" : "pat-32",
						"styles" : [ 							{
								"name" : "Jamoma_highlighted_orange",
								"default" : 								{
									"accentcolor" : [ 1.0, 0.5, 0.0, 1.0 ]
								}
,
								"parentstyle" : "",
								"multi" : 0
							}
 ]
					}
,
					"patching_rect" : [ 165.0, 242.0, 50.0, 22.0 ],
					"saved_object_attributes" : 					{
						"globalpatchername" : ""
					}
,
					"text" : "p Basic"
				}

			}
,
{
	"box": {
		"id": "obj-prism",
		"maxclass": "newobj",
		"numinlets": 0,
		"numoutlets": 0,
		"patcher": {
			"fileversion": 1,
			"appversion": {
				"major": 9,
				"minor": 2,
				"revision": 0,
				"architecture": "x64",
				"modernui": 1
			},
			"classnamespace": "box",
			"rect": [
				0.0,
				26.0,
				968.0,
				923.0
			],
			"boxes": [
				{
					"box": {
						"id": "obj-1",
						"maxclass": "jsui",
						"patching_rect": [
							8.0,
							7.0,
							674.0,
							142.0
						],
						"border": 0,
						"filename": "helpdetails.js",
						"ignoreclick": 1,
						"jsarguments": [
							"ossia.client"
						],
						"numinlets": 1,
						"numoutlets": 1,
						"outlettype": [
							""
						],
						"parameter_enable": 0
					}
				},
				{
					"box": {
						"id": "obj-2",
						"maxclass": "comment",
						"patching_rect": [
							30.0,
							165.0,
							700.0,
							21.0
						],
						"fontname": "Lato Regular",
						"fontsize": 12.0,
						"numinlets": 1,
						"numoutlets": 0,
						"text": "What ossia-prism adds to ossia.client. ossia-prism, PRISM (CNRS), https://www.prism.cnrs.fr",
						"fontface": 1
					}
				},
				{
					"box": {
						"id": "obj-3",
						"maxclass": "comment",
						"patching_rect": [
							30.0,
							200.0,
							540.0,
							78.0
						],
						"fontname": "Lato Regular",
						"fontsize": 12.0,
						"numinlets": 1,
						"numoutlets": 0,
						"text": "1. The device to connect to is in this tab, in p server: click the first message to expose it on 7413 and 7414, free ports, then connect by name (2) or by address (3). Create the client without argument, as here, or with a name only: [ossia.client <name>] takes that name and connects nothing, until a connect.",
						"linecount": 5
					}
				},
				{
					"box": {
						"id": "obj-4",
						"maxclass": "message",
						"patching_rect": [
							600.0,
							200.0,
							193.0,
							22.0
						],
						"numinlets": 2,
						"numoutlets": 1,
						"outlettype": [
							""
						],
						"text": "expose oscquery 7413 7414"
					}
				},
				{
					"box": {
						"id": "obj-5",
						"maxclass": "comment",
						"patching_rect": [
							30.0,
							292.0,
							540.0,
							78.0
						],
						"fontname": "Lato Regular",
						"fontsize": 12.0,
						"numinlets": 1,
						"numoutlets": 0,
						"text": "2. connect <name> finds the OSCQuery device announced over mDNS under that name, in this Max, another Max or TouchDesigner of the same network. Until it is found, the outlet sends connect 0 <name> every second, then connect 1 <name>. Connected by name, the client takes the tree of the listener's mirror: no node is removed and created again, and the ossia.remote objects do not register again.",
						"linecount": 5
					}
				},
				{
					"box": {
						"id": "obj-6",
						"maxclass": "message",
						"patching_rect": [
							600.0,
							292.0,
							235.0,
							22.0
						],
						"numinlets": 2,
						"numoutlets": 1,
						"outlettype": [
							""
						],
						"text": "connect ossia_prism_help_server"
					}
				},
				{
					"box": {
						"id": "obj-7",
						"maxclass": "comment",
						"patching_rect": [
							30.0,
							384.0,
							540.0,
							35.0
						],
						"fontname": "Lato Regular",
						"fontsize": 12.0,
						"numinlets": 1,
						"numoutlets": 0,
						"text": "3. connect oscquery ws://<host>:<port> connects without mDNS, on a thread of its own, so that Max does not wait for the server: connect 1 once the tree is read.",
						"linecount": 2
					}
				},
				{
					"box": {
						"id": "obj-8",
						"maxclass": "message",
						"patching_rect": [
							600.0,
							384.0,
							270.0,
							22.0
						],
						"numinlets": 2,
						"numoutlets": 1,
						"outlettype": [
							""
						],
						"text": "connect oscquery ws://127.0.0.1:7414"
					}
				},
				{
					"box": {
						"id": "obj-9",
						"maxclass": "comment",
						"patching_rect": [
							30.0,
							433.0,
							540.0,
							64.0
						],
						"fontname": "Lato Regular",
						"fontsize": 12.0,
						"numinlets": 1,
						"numoutlets": 0,
						"text": "4. When the server goes away (stop 0 here), the outlet sends connect 0 with the connection parameters (by address) or disconnected (by name, once the device is no longer announced), and the client tries again every second; expose again (1): connect 1, and every ossia.remote follows again. disconnect ends the connection and the attempts.",
						"linecount": 4
					}
				},
				{
					"box": {
						"id": "obj-10",
						"maxclass": "message",
						"patching_rect": [
							600.0,
							433.0,
							60.0,
							22.0
						],
						"numinlets": 2,
						"numoutlets": 1,
						"outlettype": [
							""
						],
						"text": "stop 0"
					}
				},
				{
					"box": {
						"id": "obj-11",
						"maxclass": "message",
						"patching_rect": [
							600.0,
							463.0,
							88.0,
							22.0
						],
						"numinlets": 2,
						"numoutlets": 1,
						"outlettype": [
							""
						],
						"text": "disconnect"
					}
				},
				{
					"box": {
						"id": "obj-12",
						"maxclass": "comment",
						"patching_rect": [
							30.0,
							511.0,
							540.0,
							49.0
						],
						"fontname": "Lato Regular",
						"fontsize": 12.0,
						"numinlets": 1,
						"numoutlets": 0,
						"text": "5. @reconnect 0 turns off the attempts, after a loss and for a first connection that fails, an unreachable URL or a name not found.",
						"linecount": 3
					}
				},
				{
					"box": {
						"id": "obj-13",
						"maxclass": "message",
						"patching_rect": [
							600.0,
							511.0,
							95.0,
							22.0
						],
						"numinlets": 2,
						"numoutlets": 1,
						"outlettype": [
							""
						],
						"text": "reconnect 0"
					}
				},
				{
					"box": {
						"id": "obj-14",
						"maxclass": "message",
						"patching_rect": [
							600.0,
							541.0,
							95.0,
							22.0
						],
						"numinlets": 2,
						"numoutlets": 1,
						"outlettype": [
							""
						],
						"text": "reconnect 1"
					}
				},
				{
					"box": {
						"id": "obj-15",
						"maxclass": "comment",
						"patching_rect": [
							30.0,
							585.0,
							540.0,
							64.0
						],
						"fontname": "Lato Regular",
						"fontsize": 12.0,
						"numinlets": 1,
						"numoutlets": 0,
						"text": "6. @bundle groups what ossia.remote, ossia.parameter and ossia.view push during a tick of the scheduler: the values leave at the end of the tick, in OSC bundles of at most 1400 bytes over UDP, one bundle over the WebSocket for a @critical parameter. -1 (the default): bundles over OSCQuery; 0: one message per value.",
						"linecount": 4
					}
				},
				{
					"box": {
						"id": "obj-16",
						"maxclass": "message",
						"patching_rect": [
							600.0,
							585.0,
							81.0,
							22.0
						],
						"numinlets": 2,
						"numoutlets": 1,
						"outlettype": [
							""
						],
						"text": "bundle -1"
					}
				},
				{
					"box": {
						"id": "obj-17",
						"maxclass": "message",
						"patching_rect": [
							600.0,
							615.0,
							74.0,
							22.0
						],
						"numinlets": 2,
						"numoutlets": 1,
						"outlettype": [
							""
						],
						"text": "bundle 0"
					}
				},
				{
					"box": {
						"id": "obj-18",
						"maxclass": "newobj",
						"patching_rect": [
							600.0,
							669.0,
							85.0,
							22.0
						],
						"text": "ossia.client",
						"numinlets": 1,
						"numoutlets": 1,
						"outlettype": [
							""
						]
					}
				},
				{
					"box": {
						"id": "obj-19",
						"maxclass": "newobj",
						"patching_rect": [
							600.0,
							709.0,
							80.0,
							22.0
						],
						"text": "print client",
						"numinlets": 1,
						"numoutlets": 0,
						"outlettype": []
					}
				},
				{
					"box": {
						"id": "obj-20",
						"maxclass": "newobj",
						"patching_rect": [
							805.0,
							200.0,
							140.0,
							22.0
						],
						"text": "s prism_help_expose",
						"numinlets": 1,
						"numoutlets": 0,
						"outlettype": []
					}
				},
				{
					"box": {
						"id": "obj-21",
						"maxclass": "newobj",
						"patching_rect": [
							672.0,
							433.0,
							140.0,
							22.0
						],
						"text": "s prism_help_expose",
						"numinlets": 1,
						"numoutlets": 0,
						"outlettype": []
					}
				},
				{
					"box": {
						"id": "obj-22",
						"maxclass": "comment",
						"patching_rect": [
							600.0,
							744.0,
							170.0,
							20.0
						],
						"fontname": "Lato Regular",
						"fontsize": 12.0,
						"numinlets": 1,
						"numoutlets": 0,
						"text": "client: ossia.remote level",
						"linecount": 1
					}
				},
				{
					"box": {
						"id": "obj-23",
						"maxclass": "comment",
						"patching_rect": [
							780.0,
							744.0,
							170.0,
							20.0
						],
						"fontname": "Lato Regular",
						"fontsize": 12.0,
						"numinlets": 1,
						"numoutlets": 0,
						"text": "server: its parameter level",
						"linecount": 1
					}
				},
				{
					"box": {
						"id": "obj-24",
						"maxclass": "flonum",
						"patching_rect": [
							600.0,
							769.0,
							60.0,
							22.0
						],
						"numinlets": 1,
						"numoutlets": 2,
						"outlettype": [
							"",
							"bang"
						],
						"parameter_enable": 0
					}
				},
				{
					"box": {
						"id": "obj-25",
						"maxclass": "newobj",
						"patching_rect": [
							600.0,
							799.0,
							120.0,
							22.0
						],
						"text": "ossia.remote level",
						"numinlets": 1,
						"numoutlets": 3,
						"outlettype": [
							"",
							"",
							""
						]
					}
				},
				{
					"box": {
						"id": "obj-26",
						"maxclass": "flonum",
						"patching_rect": [
							600.0,
							829.0,
							60.0,
							22.0
						],
						"numinlets": 1,
						"numoutlets": 2,
						"outlettype": [
							"",
							"bang"
						],
						"parameter_enable": 0
					}
				},
				{
					"box": {
						"id": "obj-27",
						"maxclass": "flonum",
						"patching_rect": [
							780.0,
							769.0,
							60.0,
							22.0
						],
						"numinlets": 1,
						"numoutlets": 2,
						"outlettype": [
							"",
							"bang"
						],
						"parameter_enable": 0
					}
				},
				{
					"box": {
						"id": "obj-28",
						"maxclass": "newobj",
						"patching_rect": [
							780.0,
							799.0,
							125.0,
							22.0
						],
						"text": "s prism_help_level",
						"numinlets": 1,
						"numoutlets": 0,
						"outlettype": []
					}
				},
				{
					"box": {
						"id": "obj-29",
						"maxclass": "newobj",
						"patching_rect": [
							780.0,
							829.0,
							150.0,
							22.0
						],
						"text": "r prism_help_level_out",
						"numinlets": 0,
						"numoutlets": 1,
						"outlettype": [
							""
						]
					}
				},
				{
					"box": {
						"id": "obj-30",
						"maxclass": "flonum",
						"patching_rect": [
							780.0,
							859.0,
							60.0,
							22.0
						],
						"numinlets": 1,
						"numoutlets": 2,
						"outlettype": [
							"",
							"bang"
						],
						"parameter_enable": 0
					}
				},
				{
					"box": {
						"id": "obj-31",
						"maxclass": "newobj",
						"numinlets": 0,
						"numoutlets": 0,
						"patcher": {
							"fileversion": 1,
							"appversion": {
								"major": 9,
								"minor": 2,
								"revision": 0,
								"architecture": "x64",
								"modernui": 1
							},
							"classnamespace": "box",
							"rect": [
								100.0,
								100.0,
								420.0,
								300.0
							],
							"boxes": [
								{
									"box": {
										"id": "obj-1",
										"maxclass": "newobj",
										"patching_rect": [
											20.0,
											20.0,
											140.0,
											22.0
										],
										"text": "r prism_help_expose",
										"numinlets": 0,
										"numoutlets": 1,
										"outlettype": [
											""
										]
									}
								},
								{
									"box": {
										"id": "obj-2",
										"maxclass": "newobj",
										"patching_rect": [
											20.0,
											60.0,
											230.0,
											22.0
										],
										"text": "ossia.device ossia_prism_help_server",
										"numinlets": 1,
										"numoutlets": 1,
										"outlettype": [
											""
										]
									}
								},
								{
									"box": {
										"id": "obj-3",
										"maxclass": "newobj",
										"patching_rect": [
											20.0,
											100.0,
											85.0,
											22.0
										],
										"text": "print server",
										"numinlets": 1,
										"numoutlets": 0,
										"outlettype": []
									}
								},
								{
									"box": {
										"id": "obj-4",
										"maxclass": "newobj",
										"patching_rect": [
											20.0,
											150.0,
											125.0,
											22.0
										],
										"text": "r prism_help_level",
										"numinlets": 0,
										"numoutlets": 1,
										"outlettype": [
											""
										]
									}
								},
								{
									"box": {
										"id": "obj-5",
										"maxclass": "newobj",
										"patching_rect": [
											20.0,
											190.0,
											300.0,
											22.0
										],
										"text": "ossia.parameter level @type float @min 0. @max 1.",
										"numinlets": 1,
										"numoutlets": 3,
										"outlettype": [
											"",
											"",
											""
										]
									}
								},
								{
									"box": {
										"id": "obj-6",
										"maxclass": "newobj",
										"patching_rect": [
											20.0,
											230.0,
											150.0,
											22.0
										],
										"text": "s prism_help_level_out",
										"numinlets": 1,
										"numoutlets": 0,
										"outlettype": []
									}
								}
							],
							"lines": [
								{
									"patchline": {
										"source": [
											"obj-1",
											0
										],
										"destination": [
											"obj-2",
											0
										]
									}
								},
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
								}
							]
						},
						"patching_rect": [
							780.0,
							669.0,
							70.0,
							22.0
						],
						"text": "p server"
					}
				},
				{
					"box": {
						"id": "obj-32",
						"maxclass": "comment",
						"patching_rect": [
							30.0,
							669.0,
							540.0,
							49.0
						],
						"fontname": "Lato Regular",
						"fontsize": 12.0,
						"numinlets": 1,
						"numoutlets": 0,
						"text": "p server holds the device of this tab, ossia.device ossia_prism_help_server, and its parameter level. The number boxes on the right set and show the level of the client and of the server: a value set on either side reaches the other.",
						"linecount": 3
					}
				}
			],
			"lines": [
				{
					"patchline": {
						"source": [
							"obj-18",
							0
						],
						"destination": [
							"obj-19",
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
							"obj-20",
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
							"obj-18",
							0
						],
						"midpoints": [
							609.5,
							319.0,
							588.0,
							319.0,
							588.0,
							661.0,
							609.5,
							661.0
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
							"obj-18",
							0
						],
						"midpoints": [
							609.5,
							411.0,
							588.0,
							411.0,
							588.0,
							661.0,
							609.5,
							661.0
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
							"obj-21",
							0
						]
					}
				},
				{
					"patchline": {
						"source": [
							"obj-11",
							0
						],
						"destination": [
							"obj-18",
							0
						],
						"midpoints": [
							609.5,
							490.0,
							588.0,
							490.0,
							588.0,
							661.0,
							609.5,
							661.0
						]
					}
				},
				{
					"patchline": {
						"source": [
							"obj-13",
							0
						],
						"destination": [
							"obj-18",
							0
						],
						"midpoints": [
							609.5,
							538.0,
							588.0,
							538.0,
							588.0,
							661.0,
							609.5,
							661.0
						]
					}
				},
				{
					"patchline": {
						"source": [
							"obj-14",
							0
						],
						"destination": [
							"obj-18",
							0
						],
						"midpoints": [
							609.5,
							568.0,
							588.0,
							568.0,
							588.0,
							661.0,
							609.5,
							661.0
						]
					}
				},
				{
					"patchline": {
						"source": [
							"obj-16",
							0
						],
						"destination": [
							"obj-18",
							0
						],
						"midpoints": [
							609.5,
							612.0,
							588.0,
							612.0,
							588.0,
							661.0,
							609.5,
							661.0
						]
					}
				},
				{
					"patchline": {
						"source": [
							"obj-17",
							0
						],
						"destination": [
							"obj-18",
							0
						],
						"midpoints": [
							609.5,
							642.0,
							588.0,
							642.0,
							588.0,
							661.0,
							609.5,
							661.0
						]
					}
				},
				{
					"patchline": {
						"source": [
							"obj-24",
							0
						],
						"destination": [
							"obj-25",
							0
						]
					}
				},
				{
					"patchline": {
						"source": [
							"obj-25",
							0
						],
						"destination": [
							"obj-26",
							0
						]
					}
				},
				{
					"patchline": {
						"source": [
							"obj-27",
							0
						],
						"destination": [
							"obj-28",
							0
						]
					}
				},
				{
					"patchline": {
						"source": [
							"obj-29",
							0
						],
						"destination": [
							"obj-30",
							0
						]
					}
				}
			],
			"showontab": 1
		},
		"patching_rect": [
			395.0,
			242.0,
			58.0,
			22.0
		],
		"text": "p PRISM"
	}
}
 ],
		"lines" : [  ],
		"originid" : "pat-20",
		"dependency_cache" : [ 			{
				"name" : "helpdetails.js",
				"bootpath" : "C74:/help/resources",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "ossia.client.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "ossia.device.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "ossia.model.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "ossia.parameter.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "ossia.remote.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "ossia.view.mxo",
				"type" : "iLaX"
			}
 ],
		"autosave" : 0
	}

}
