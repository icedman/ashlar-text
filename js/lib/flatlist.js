import React from "react";
import View from "./view";
import Text from "./text";
import ScrollView from "./scrollview";
import { v4 as uuid } from "uuid";
import qt from "./engine";

const FlatList_ = props => {
  const [state, setState] = React.useState({
    renderItem: props.renderItem
  });

  const data = props.data || [];
  const Item = state.renderItem;
  const keyExtractor = props.keyExtractor || ((item, index) => item + index);
  const renderedItems = data.map((item, index) => {
    return (
      <Item
        item={item}
        {...(props.extraData || {})}
        key={keyExtractor(item, index)}
      />
    );
  });

  return (
    <ScrollView {...props}>
      <View style={{ flexDirection: "column", alignItems: "flex-start" }}>
        {renderedItems}
      </View>
    </ScrollView>
  );
};

const FlatList = React.memo(FlatList_);

export default FlatList;
