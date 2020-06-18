import React from "react";
import ReactDOM from "react-dom";
import View from "./view";

const Text = props => {
  const [renderedText, setRenderedText] = React.useState(null);
  let ref = React.useRef();

  React.useEffect(() => {
    setRenderedText(ref.current.innerHTML);
  }, [props.children]);

  const more = {};
  more.renderedText = renderedText;

  return (
    <View {...props} {...more} type="Text">
      <div ref={ref}>{props.children}</div>
    </View>
  );
};

export default Text;
