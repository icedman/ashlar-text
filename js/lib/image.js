import React from "react";
import View from "./view";

const Image = props => {
  return (
    <View {...props} type="Image">
      <div style={{ display: "inline-block" }}>
        <img src={props.source} style={{ maxWidth: "100%", height: "auto" }} />
      </div>
    </View>
  );
};

export default Image;
