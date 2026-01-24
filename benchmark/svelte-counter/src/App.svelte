<script>
  import Counter from './Counter.svelte'
  
  let score = $state(0)
  
  function handleChange(delta) {
    score += delta
  }
</script>

<div class="app">
  <h1>Score: {score}</h1>
  <Counter 
    label="Player" 
    value={score} 
    onChange={handleChange} 
  />
  {#if score >= 10}
    <p class="win">🎉 You win!</p>
  {/if}
</div>

<style>
  .app {
    padding: 24px;
    max-width: 400px;
    margin: 0 auto;
  }

  .app h1 {
    color: #1a73e8;
    margin-bottom: 24px;
  }

  .win {
    color: #34a853;
    font-weight: bold;
    font-size: 1.25rem;
    margin-top: 16px;
    padding: 12px;
    background: #e8f5e9;
    border-radius: 8px;
  }
</style>
