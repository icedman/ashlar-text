import React from "react";
import View from "./view";

const Switch = props => {
  return <View {...props} type="Button">{props.value===true?'on':'off'}</View>;
};

export default Switch;
