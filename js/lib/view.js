import React from "react";
import clsx from "clsx";
import { v4 as uuid } from "uuid";
import qt from "./engine";

const getOrder = id => {
  let node = document.querySelector(`[id="${id}"]`);
  if (!node) {
    return -1;
  }
  let i = 0;
  while ((node = node.previousSibling) != null) i++;
  return i;
};

const getParentId = id => {
  let node = document.querySelector(`[id="${id}"]`);
  if (!node) {
    return null;
  }
  node = node.parentElement;
  while (node) {
    if (!node.classList.contains("qt")) {
      node = node.parentElement;
      continue;
    }
    return node.id;
  }
  return null;
};

const View_ = props => {
  const [state, setState] = React.useState({
    type: props.type || "View",
    id: props.id || uuid(),
    persistent: props.id
  });

  let className = clsx("qt", state.type, props.className);
  let style = {
    display: "flex",
    flexDirection: "column",
    ...(props.style || {})
  };
  let uiInfo = { ...props, ...state, className: className };

  const setIds = () => {
    let parentId = getParentId(state.id) || "";
    let order = getOrder(state.id) || -1;
    setState({
      ...state,
      parent: parentId,
      order: order
    });
  };

  React.useEffect(() => {
    setTimeout(setIds, 0);
    qt.mount(uiInfo);
    return () => {
      qt.unmount(uiInfo);
    };
  }, [uiInfo.id]);

  qt.update(uiInfo);

  return (
    <div id={uiInfo.id} type={uiInfo.type} className={uiInfo.className}>
      {props.children}
    </div>
  );
};

const View = React.memo(View_);

export default View;
