OSCMessenger : UGen {
	*kr {|portNumber, oscAddress, trigger, values, doneAddress=nil, doneValue=1|
		if(values.containsSeqColl.not) { values = values.bubble };
		if(trigger.isFloat) { trigger = Impulse.kr(trigger) };
		trigger = trigger.bubble;
		[portNumber, trigger, oscAddress, doneAddress, doneValue, values].flop.do { |args|
			this.new1('control', *args);
		};
	}

	checkInputs {
		^this.checkValidInputs;
	}

	*new1 {|rate, portNumber, trigger, oscAddress, doneAddress, doneValue, values|
		var oscAddressAscii;
		var doneAddressAscii;
		var args;

		doneAddress = if(doneAddress.isNil, {""}, {doneAddress}).asString;

		oscAddressAscii = oscAddress.ascii;
		doneAddressAscii = doneAddress.ascii;

		args = [
			rate,
			portNumber,
			trigger,
			oscAddressAscii.size,
			doneAddressAscii.size,
			doneValue,
		].addAll(oscAddressAscii).addAll(doneAddressAscii).addAll(values);

		^super.new1(*args);
	}
}
