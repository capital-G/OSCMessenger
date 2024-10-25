OSCMessenger : UGen {
	*kr {|portNumber, oscAddress, values, trigger=60, doneAddress=nil, doneValue=1, host="127.0.0.1", appendNodeId=false|
		if(values.containsSeqColl.not) { values = values.bubble };
		if(trigger.isFloat.or(trigger.isInteger)) { trigger = Impulse.kr(trigger) };
		trigger = trigger.bubble;
		[portNumber, oscAddress, values, trigger, doneAddress, doneValue, host, appendNodeId].flop.do { |args|
			this.new1('control', *args);
		};
	}

	checkInputs {
		^this.checkValidInputs;
	}

	*new1 {|rate, portNumber, oscAddress, values, trigger, doneAddress, doneValue, host, appendNodeId|
		var oscAddressAscii;
		var doneAddressAscii;
		var hostAscii;
		var args;

		doneAddress = if(doneAddress.isNil, {""}, {doneAddress}).asString;

		oscAddressAscii = oscAddress.ascii;
		doneAddressAscii = doneAddress.ascii;
		hostAscii = host.ascii;

		args = [
			rate,
			portNumber,
			trigger,
			oscAddressAscii.size,
			doneAddressAscii.size,
			doneValue,
			hostAscii.size,
			appendNodeId.asInteger,
		].addAll(oscAddressAscii).addAll(doneAddressAscii).addAll(hostAscii).addAll(values);

		^super.new1(*args);
	}
}

+ UGen {
	oscMessenger {|portNumber, oscAddress, trigger=60, doneAddress=nil, doneValue=1, host="127.0.0.1", appendNodeId=false|
		OSCMessenger.kr(
			portNumber: portNumber,
			oscAddress: oscAddress,
			values: this,
			trigger: trigger,
			doneAddress: doneAddress,
			doneValue: doneValue,
			host: host,
			appendNodeId: appendNodeId,
		)
		^this;
	}
}
