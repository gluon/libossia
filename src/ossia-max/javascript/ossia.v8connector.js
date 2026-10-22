// ossia.v8connector.js: helper for v8 and v8ui scripts around [ossia.v8connector].
//
// The script reaches the object without a patch cord, through its scripting
// name (the "varname" set in the inspector), or through a Maxobj:
//
//   const OssiaConnector = require("ossia.v8connector");
//   const synth = new OssiaConnector("synth");           // scripting name, or a Maxobj
//   synth.declare({
//     gain:   { type: "float", min: -60, max: 6, unit: "gain.db", default: -12 },
//     cutoff: { type: "float", range: [20, 20000], unit: "time.hz" },
//     voices: { type: "int", min: 1, max: 16, default: 4 },
//   });
//   synth.on("gain", (v) => { gain = v; mgraphics.redraw(); });
//   synth.set("gain", -6);
//   synth.remove("voices");
//
// bind subscribes to a parameter that exists elsewhere, declared by another
// instance or by an ossia.parameter, without creating anything. The address
// follows the rules of ossia.remote: relative to the enclosing model or
// device, absolute with a leading slash, or device:/address.
//
//   const mirror = new OssiaConnector("mirror");
//   mirror.bind("synth/gain");
//   mirror.on("synth/gain", (v) => { gain = v; });
//   mirror.set("synth/gain", -6);                 // pushes to the owner's parameter
//   mirror.unbind("synth/gain");
//
// Values that change by another source (a remote client, another ossia object,
// another script) leave the object's left outlet as "<address> <values...>".
// Patch that outlet into the script's inlet and forward them from anything():
//
//   function anything() { synth.dispatch(messagename, arrayfromargs(arguments)); }
//
// A numeric type (float, int, vec2f, vec3f, vec4f) needs min and max, or
// range: the object refuses the declaration otherwise.

function OssiaConnector(target) {
  this.handlers = {};
  this.target = null;
  this.name = null;
  if (typeof target === "string") {
    this.name = target;
  } else if (target) {
    this.target = target;
  }
}

// Resolve the object lazily: the scripting name may be set after the script loads.
OssiaConnector.prototype.object = function () {
  if (this.target) return this.target;
  if (!this.name) return null;
  var p = this.patcher();
  while (p) {
    var obj = p.getnamed(this.name);
    if (obj) {
      this.target = obj;
      return obj;
    }
    p = p.parentpatcher;
  }
  return null;
};

OssiaConnector.prototype.patcher = function () {
  if (typeof patcher !== "undefined" && patcher) return patcher;
  if (typeof this_patcher !== "undefined") return this_patcher;
  return null;
};

OssiaConnector.prototype.send = function (args) {
  var obj = this.object();
  if (!obj) {
    error("ossia.v8connector: no object named '" + this.name + "' in this patcher or above\n");
    return false;
  }
  obj.message.apply(obj, args);
  return true;
};

// declare({ address: { type, min, max, range, unit, default, clip, mode,
//                      description, tags, priority, rate, critical, invisible,
//                      repetitions, enable, recall_safe } })
// Each entry becomes one "add" message with the same attribute names as ossia.parameter.
OssiaConnector.prototype.declare = function (spec) {
  var addresses = Object.keys(spec);
  for (var i = 0; i < addresses.length; i++) {
    var address = addresses[i];
    var attrs = spec[address] || {};
    var args = ["add", address];
    var names = Object.keys(attrs);
    for (var j = 0; j < names.length; j++) {
      var value = attrs[names[j]];
      if (value === undefined || value === null) continue;
      args.push("@" + names[j]);
      if (Array.isArray(value)) {
        for (var k = 0; k < value.length; k++) args.push(value[k]);
      } else if (typeof value === "boolean") {
        args.push(value ? 1 : 0);
      } else {
        args.push(value);
      }
    }
    this.send(args);
  }
};

OssiaConnector.prototype.add = function (address, attrs) {
  var spec = {};
  spec[address] = attrs;
  this.declare(spec);
};

OssiaConnector.prototype.set = function (address) {
  var args = ["set", address];
  for (var i = 1; i < arguments.length; i++) {
    var v = arguments[i];
    if (Array.isArray(v)) {
      for (var k = 0; k < v.length; k++) args.push(v[k]);
    } else if (typeof v === "boolean") {
      args.push(v ? 1 : 0);
    } else {
      args.push(v);
    }
  }
  this.send(args);
};

OssiaConnector.prototype.remove = function (address) {
  this.send(["remove", address]);
};

OssiaConnector.prototype.clear = function () {
  this.send(["clear"]);
};

// bind(address): follow a parameter that exists elsewhere; set and on work on it.
OssiaConnector.prototype.bind = function (address) {
  this.send(["bind", address]);
};

OssiaConnector.prototype.unbind = function (address) {
  this.send(["unbind", address]);
};

// on(address, handler): handler(values...) runs when the value changes by another source.
OssiaConnector.prototype.on = function (address, handler) {
  this.handlers[address] = handler;
};

OssiaConnector.prototype.off = function (address) {
  delete this.handlers[address];
};

// dispatch(address, values): call from anything() with the messages of the left outlet.
OssiaConnector.prototype.dispatch = function (address, values) {
  var handler = this.handlers[address];
  if (!handler) return false;
  handler.apply(null, values);
  return true;
};

if (typeof module !== "undefined") module.exports = OssiaConnector;
