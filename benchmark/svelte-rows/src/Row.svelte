<script>
  let { item, isSelected, onSelect, onRemove } = $props()
</script>

<li class="row" class:selected={isSelected}>
  <span class="row-id">{item.id}</span>
  <span class="row-label" onclick={() => onSelect(item.id)}>{item.label}</span>
  <span class="row-actions">
    <button onclick={() => onRemove(item.id)}>×</button>
  </span>
</li>
