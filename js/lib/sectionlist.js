import React from "react";
import View from "./view";
import Text from "./text";
import ScrollView from "./scrollview";
import { v4 as uuid } from "uuid";
import qt from "./engine";

const SectionList_ = props => {
  const [state, setState] = React.useState({
    renderItem: props.renderItem,
    renderSectionHeader: props.renderSectionHeader
  });

  let data = props.data || [];
  const Item = state.renderItem;
  const Header = state.renderSectionHeader;
  const keyExtractor = props.keyExtractor || ((item, index) => item + index);
  
  if (!data.map) {
      data = [];
  }
  
  const renderedItems = data.map((section, sectionIndex) => {
    const sectionItems = section.data.map((item, index) => {
      return (
        <Item
          item={item}
          {...(props.extraData || {})}
          key={keyExtractor(item, index)}
        />
      );
    });

    return (
      <React.Fragment
        key={`seclist-${sectionIndex}` + keyExtractor(section, sectionIndex)}
      >
        <Header section={section} {...(props.extraData || {})} />
        {sectionItems}
      </React.Fragment>
    );
  });

  return (
    <ScrollView {...props}>
      <View style={{ 'flex-direction': 'column', 'align-items': 'flex-start' }}>
        {renderedItems}
      </View>
    </ScrollView>
  );
};

const SectionList = React.memo(SectionList_);

export default SectionList;
