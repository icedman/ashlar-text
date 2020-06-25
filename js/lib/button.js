import React from "react";
import View from "./view";

const Button = props => {
  return (
    <View {...props} type="Button">
      {props.text}
    </View>
  );
};

export default Button;
